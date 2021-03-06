#define MODULE_LOG_PREFIX "newcamd"

#include "globals.h"
#ifdef MODULE_NEWCAMD
#include "cscrypt/des.h"
#include "cscrypt/md5.h"
#include "module-newcamd.h"
#include "oscam-array.h"
#include "oscam-config.h"
#include "oscam-conf-chk.h"
#include "oscam-chk.h"
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-emm.h"
#include "oscam-net.h"
#include "oscam-reader.h"
#include "oscam-string.h"
#include "oscam-time.h"
// csp 0.8.9 (default: 400). This is CWS_NETMSGSIZE. The old default was 240
#define CWS_NETMSGSIZE 	400
#define NCD_CLIENT_ID 	0x8888

#define CWS_FIRSTCMDNO 	0xe0
typedef enum
{
	MSG_CLIENT_2_SERVER_LOGIN = CWS_FIRSTCMDNO,
	MSG_CLIENT_2_SERVER_LOGIN_ACK,
	MSG_CLIENT_2_SERVER_LOGIN_NAK,
	MSG_CARD_DATA_REQ,
	MSG_CARD_DATA,
	MSG_SERVER_2_CLIENT_NAME,
	MSG_SERVER_2_CLIENT_NAME_ACK,
	MSG_SERVER_2_CLIENT_NAME_NAK,
	MSG_SERVER_2_CLIENT_LOGIN,
	MSG_SERVER_2_CLIENT_LOGIN_ACK,
	MSG_SERVER_2_CLIENT_LOGIN_NAK,
	MSG_ADMIN,
	MSG_ADMIN_ACK,
	MSG_ADMIN_LOGIN,
	MSG_ADMIN_LOGIN_ACK,
	MSG_ADMIN_LOGIN_NAK,
	MSG_ADMIN_COMMAND,
	MSG_ADMIN_COMMAND_ACK,
	MSG_ADMIN_COMMAND_NAK,
	MSG_KEEPALIVE 							= CWS_FIRSTCMDNO + 0x1d,
	MSG_SERVER_2_CLIENT_OSD 			= 0xd1,
	MSG_SERVER_2_CLIENT_ALLCARDS 		= 0xd2,
	MSG_SERVER_2_CLIENT_ADDCARD 		= 0xd3,
	MSG_SERVER_2_CLIENT_REMOVECARD 	= 0xd4,
	MSG_SERVER_2_CLIENT_CHANGE_KEY 	= 0xd5,
	MSG_SERVER_2_CLIENT_GET_VERSION 	= 0xd6,
	MSG_SERVER_2_CLIENT_ADDSID 		= 0xd7,
	MSG_CLIENT_2_SERVER_CARDDISCOVER = 0xd8
} net_msg_type_t;

typedef enum
{
	COMMTYPE_CLIENT,
	COMMTYPE_SERVER
} comm_type_t;


typedef struct custom_data
{
	uint16_t sid;
	uint16_t caid;
	int32_t 	provid;
	uchar 	x;
} custom_data_t;

#define REQ_SIZE  2

#if defined(MODULE_AVAMGCAMD)
static int32_t avatarcamd_activation(struct s_reader *reader);
#endif

// sky(n)
int32_t network_recvpoll(int32_t handle, uint8_t *buf, int32_t len)
{
	struct pollfd pfd;
	int32_t rc;

	while (1)
	{
		pfd.fd = handle;
		pfd.events = POLLIN | POLLPRI;
		rc = poll(&pfd, 1, 3000);
		if (rc < 0) {
			if (errno==EINTR) continue;
			return (-1); // error!!
		}

		if (rc == 1) {
			if (pfd.revents & POLLHUP) return (-1); //hangup = error!!
			break;
		}
		return (-2); // timeout!!
	}
	return recv(handle, buf, len, MSG_WAITALL);
}

static int32_t network_message_send(int32_t handle, uint16_t *netMsgId, uint8_t *buffer,
                                int32_t  len, uint8_t *deskey, comm_type_t commType,
                                uint16_t sid, custom_data_t *cd)
{
	uint8_t netbuf[CWS_NETMSGSIZE];
	int32_t head_size;
	struct s_client *cl = cur_client();

	head_size = (cl->ncd_proto==NCD_524)?8:12;

	if (len < 3 || len + head_size > CWS_NETMSGSIZE || handle < 0)
		return -1;
	buffer[1] = (buffer[1] & 0xf0) | (((len - 3) >> 8) & 0x0f);
	buffer[2] = (len - 3) & 0xff;
	memcpy(netbuf+head_size, buffer, len);
	len += head_size;
	if (netMsgId) {
		if (commType==COMMTYPE_CLIENT) (*netMsgId)++;
		netbuf[2] = (*netMsgId) >> 8;
		netbuf[3] = (*netMsgId) & 0xff;
	}
	else {
		netbuf[2] = netbuf[3] = 0;
	}
	memset(netbuf+4, 0, (cl->ncd_proto==NCD_524)?4:8);
	if (sid)
	{
		if (cl->reader && cl->reader->ncd_disable_server_filt &&
			sid != NCD_CLIENT_ID && cl->ncd_proto != NCD_524)
		{	// mgclient send header
		  	memcpy(netbuf+4, cl->ncd_header+4, 7);
		}
		netbuf[(cl->ncd_proto==NCD_524)?6:4] = (uchar)(sid>>8); 	// sid
		netbuf[(cl->ncd_proto==NCD_524)?7:5] = (uchar)(sid);
	}
//	if ((!ncd_proto==NCD_524) && (buffer[0] >= 0xd1) && (buffer[0]<= 0xd8)) // extended proto for mg
//	{
//  	cs_log_dbg(D_CLIENT, "newcamd: extended: msg");
		if (cd)
		{
		  cs_log_dbg(D_CLIENT, "newcamd: has cd");
		  netbuf[ 4] = cd->sid  >> 8;
		  netbuf[ 5] = cd->sid   & 0xff;
		  netbuf[ 6] = cd->caid >> 8;
		  netbuf[ 7] = cd->caid  & 0xff;
		  netbuf[ 8] =(cd->provid >> 16) & 0xff;
		  netbuf[ 9] =(cd->provid >> 8) & 0xff;
		  netbuf[10] = cd->provid  & 0xff;
		}
//	}
	if (NCD_525 == cl->ncd_proto && cfg.ncd_mgclient &&
		 MSG_CLIENT_2_SERVER_LOGIN_ACK == buffer[0])
	{
		netbuf[ 4] = 0x6E; // From ExtNewcamSession.java CSP line 65-66 getLoginOkMsg()
		netbuf[ 5] = 0x73;
		netbuf[11] = 0x14;
	}
	if (NCD_525 == cl->ncd_proto &&
		 MSG_SERVER_2_CLIENT_ADDSID == buffer[0])
	{
		netbuf[11] = 0x14;
	}
//	if (buffer[0]==MSG_CLIENT_2_SERVER_LOGIN && cur_client()->reader->ncd_disable_server_filt) {
//    netbuf[11] = 0x11; // From ChameleonCwsConnector.java CSP line 59-61 run()
//	}

#if defined(MODULE_AVAMGCAMD)
	if (buffer[0]==MSG_CLIENT_2_SERVER_LOGIN && IS_AVATARCAMD(cl->reader->ncd_exprotocol))
	{
		memset(netbuf+4, 0, 8);
		netbuf[4] = 0x76; // client ID
		netbuf[5] = 0x45; // client ID
	}
#endif

	netbuf[0] = (len - 2) >> 8;
	netbuf[1] = (len - 2) & 0xff;
	cs_log_dump_dbg(D_CLIENT, netbuf, len, "send %d bytes to %s", len, remote_txt());
	if ((len = des_encrypt(netbuf, len, deskey)) < 0) return -1;

	netbuf[0] = (len - 2) >> 8;
	netbuf[1] = (len - 2) & 0xff;
	return send(handle, netbuf, len, 0);
}

static int32_t send_sid_list(void)
{
	struct s_client *cl = cur_client();

	if (1 != cl->ftab.nfilts || !cl->sidtabs.no || !cfg.ncd_ptab.ports[cl->port_idx].ncd)
	{
		cs_log("SID list will not be send to mgcamd client.");
		return 0;
	}

	uchar mbuf[CWS_NETMSGSIZE];
	int32_t n = 0, nr = 0, portion_sid_num = 0, i = 0, sid_num = 0,
	portion_num = 0;
	SIDTAB *sidtab = 0;
	custom_data_t cd;

	cs_log_dbg(D_TRACE,"Send SID list to mgcamd client.");
	memset(&cd, 0, sizeof(cd));
	FILTER *pfilts = cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts;

	/*memset(mbuf, 0, sizeof(mbuf));*/ // not nessesery

	for (nr=0, sidtab=cfg.sidtab; sidtab; sidtab=sidtab->next, nr++)
	{
		if ((cl->sidtabs.no&((SIDTABBITS)1<<nr)) && (sidtab->num_caid | sidtab->num_provid | sidtab->num_srvid))
		{
			for (n = 0; n < pfilts[0].nprids; n++)
			{
				if (chk_srvid_match_by_caid_prov(pfilts[0].caid, pfilts[0].prids[n], sidtab))
				{
					for (i = 0; i < sidtab->num_srvid; i++)
					{
						// First SID goes to header
						if (0 == portion_sid_num)
						{
							cd.sid = sidtab->srvid[i]; // first sid
							cd.caid = cfg.ncd_ptab.ports[cl->port_idx].s_port;   //assigned port
							cd.provid = 0x1; // mark as deny
						}
						mbuf[portion_sid_num*3] = (uchar)(sidtab->srvid[i] >> 8);
						mbuf[portion_sid_num*3 +1] = (uchar)(sidtab->srvid[i] & 0xFF);
						mbuf[portion_sid_num*3 +2] = 0x1; // mark as deny

						++sid_num;
						++portion_sid_num;

						if (portion_sid_num >= 50)
						{
							++portion_num;
							cs_log_dump_dbg(0x0800, mbuf, (portion_sid_num)*3, "Portion %d contains %d SIDs", portion_num, portion_sid_num);
							mbuf[0] = MSG_SERVER_2_CLIENT_ADDSID;
							mbuf[1] = 0x0;
							mbuf[2] = 0x0;
							network_message_send(cl->udp_fd, &cl->ncd_msgid,
							       mbuf, portion_sid_num*3, cl->ncd_skey, COMMTYPE_SERVER, 0, &cd);
							portion_sid_num = 0;
						}
					}

					break;
				}
			}
		}
	}
	if (portion_sid_num)
	{
		++portion_num;
		cs_log_dump_dbg(0x0800, mbuf, (portion_sid_num)*3, "Portion %d contains %d SIDs", portion_num, portion_sid_num);
		mbuf[0] = MSG_SERVER_2_CLIENT_ADDSID;
		mbuf[1] = 0x0;
		mbuf[2] = 0x0;
		network_message_send(cl->udp_fd, &cl->ncd_msgid, mbuf, portion_sid_num*3, cl->ncd_skey, COMMTYPE_SERVER, 0, &cd);
		portion_sid_num = 0;
	}

	cs_log("%d deny SIDs in the %d messages were sent to the client.", sid_num, portion_num);
	return sid_num;
}

static int32_t network_message_receive(int32_t handle, uint16_t *netMsgId, uint8_t *buffer,
                                   uint8_t *deskey, comm_type_t commType)
{
	int32_t len, ncd_off, msgid;
	uint8_t netbuf[CWS_NETMSGSIZE];
	int32_t returnLen;
	struct s_client *cl = cur_client();

	if (!buffer || handle < 0)
		return -1;
	len = recv(handle, netbuf, 2, 0);
	cs_log_dbg(D_CLIENT, "nmr(): len=%d, errno=%d", len, (len==-1) ? errno : 0);
	if (!len) {
		cs_log_dbg(D_CLIENT, "nmr: 1 return 0");
		if (commType == COMMTYPE_CLIENT)
			network_tcp_connection_close(cl->reader, "receive error1");
		else
			cs_disconnect_client(cl);
		return 0;
	}
	if (len != 2) {
		cs_log_dbg(D_CLIENT, "nmr: len!=2");
		if (commType == COMMTYPE_CLIENT)
			network_tcp_connection_close(cl->reader, "receive error2");
		else
			cs_disconnect_client(cl);
		return -1;
	}
	if (((netbuf[0] << 8) | netbuf[1]) > CWS_NETMSGSIZE - 2) {
		cs_log_dbg(D_CLIENT, "nmr: received data len=%d lonage than CWS_NETMSGSIZE=%d", ((netbuf[0] << 8) | netbuf[1]), CWS_NETMSGSIZE);
		cs_log_dbg(D_CLIENT, "nmr: 1 return -1");
		return -1;
	}

	len = recv(handle, netbuf+2, (netbuf[0] << 8) | netbuf[1], 0);
	if (!len) {
		cs_log_dbg(D_CLIENT, "nmr: 2 return 0");
		return 0;
	}
	if (len != ((netbuf[0] << 8) | netbuf[1])) {
		cs_log_dbg(D_CLIENT, "nmr: 2 return -1");
		return -1;
	}
	len += 2;
	if ((len = des_decrypt(netbuf, len, deskey)) < 11) {  // 15(newcamd525) or 11 ???
		cs_log_dbg(D_CLIENT, "nmr: can't decrypt, invalid des key?");
		cs_sleepms(2000);
		return -1;
	}
//	cs_log_dump_dbg(D_CLIENT, netbuf, len, "nmr: decrypted data, len=%d", len);
  	msgid = (netbuf[2] << 8) | netbuf[3];

	if (cl->ncd_proto==NCD_AUTO) {
		// auto detect
		int32_t l5 = (((netbuf[13] & 0x0f) << 8) | netbuf[14]) + 3;
		int32_t l4 = (((netbuf[ 9] & 0x0f) << 8) | netbuf[10]) + 3;

		if ((l5<=len-12) && ((netbuf[12]&0xF0)==0xE0 || (netbuf[12]&0xF0)==0x80))
			cl->ncd_proto = NCD_525;
		else if ((l4<=len-8) && ((netbuf[8]&0xF0)==0xE0 || (netbuf[9]&0xF0)==0x80))
			cl->ncd_proto = NCD_524;
		else {
			cs_log_dbg(D_CLIENT, "nmr: 4 return -1");
			return -1;
		}

		cs_log_dbg(D_CLIENT, "nmr: autodetect: newcamd52%d used", (cl->ncd_proto==NCD_525)?5:4);
	}

	ncd_off = (cl->ncd_proto==NCD_525)?4:0;

	returnLen = (((netbuf[9+ncd_off] & 0x0f) << 8) | netbuf[10+ncd_off]) + 3;
	if (returnLen > (len-(8+ncd_off))) {
		cs_log_dbg(D_CLIENT, "nmr: 4 return -1");
		return -1;
	}

//	cs_log_dump_dbg(D_CLIENT, netbuf, len, "nmr: decrypted data");
	if (netMsgId)
	{
		switch (commType)
		{
			case COMMTYPE_SERVER:
				*netMsgId = msgid;
				break;

			case COMMTYPE_CLIENT:
			//	if (*netMsgId != ((netbuf[2] << 8) | netbuf[3])) {
					cs_log_dbg(D_CLIENT, "nmr: netMsgId=%d, from server=%d, ", *netMsgId, msgid);
			//		return -2;
			//	}
				break;

			default:
				cs_log_dbg(D_CLIENT, "nmr: 5 return -1");
				return -1;
				break;
		}
	}
	switch (commType)
	{
		case COMMTYPE_SERVER:
			memcpy(cl->ncd_header, netbuf, (8+ncd_off));
			buffer[0]=(cl->ncd_proto==NCD_525)?netbuf[4]:netbuf[6]; // sid
			buffer[1]=(cl->ncd_proto==NCD_525)?netbuf[5]:netbuf[7];
			break;
		case COMMTYPE_CLIENT:
			memcpy(cl->ncd_header, netbuf, (8+ncd_off));
			buffer[0]=netbuf[2]; // msgid
			buffer[1]=netbuf[3];
			break;
	}

	memcpy(buffer+2, netbuf+(8+ncd_off), returnLen);
	return returnLen+2;
}

static void network_cmd_no_data_send(int32_t handle, uint16_t *netMsgId,
                                     net_msg_type_t cmd, uint8_t *deskey,
                                     comm_type_t commType)
{
	uint8_t buffer[3];

	buffer[0] = cmd;
	buffer[1] = 0;
	buffer[2] = 0;
	network_message_send(handle, netMsgId, buffer, 3, deskey, commType, 0, NULL);
}

static int32_t network_cmd_no_data_receive(int32_t handle, uint16_t *netMsgId,
                                       uint8_t *deskey, comm_type_t commType)
{
	uint8_t buffer[CWS_NETMSGSIZE];

	if (network_message_receive(handle, netMsgId, buffer, deskey, commType) != 3+2)
		return -1;
	return buffer[2];
}


#if defined(MODULE_AVAMGCAMD)
int32_t mavatarcamd_cas_addfilters(struct s_reader *rdr, uint16_t casid, uint32_t prid)
{
	int add = 0;
	int i, k;

//	if (IS_IRDETO(casid)) prid = 0x0;
	if (IS_VIACESS(casid) && prid==0) return 0;
	if (IS_SEKA(casid) && prid==0) return 0;

	for (i=0; i<rdr->ftab.nfilts; i++)
	{
		if (rdr->ftab.filts[i].caid == casid) break;
	}
	if (i==rdr->ftab.nfilts)
	{
	//	if (i>(CS_MAXFILTERS-1)) break;
		if (i>(CS_MAXFILTERS-1))
		{
			mycs_debug(D_ADB, "ftabs:CA{%04X.%08X} CS_MAXFILTERS exceed", casid, prid);
			return -1;
		}
		rdr->ftab.filts[i].caid = casid;
		rdr->ftab.filts[i].prids[0] = prid;
		rdr->ftab.filts[i].nprids = 1;
		rdr->ftab.nfilts++;
		add = 1;
	}
	else
	{
		for (k=0; k<rdr->ftab.filts[i].nprids; k++)
		{
			if (rdr->ftab.filts[i].prids[k] == prid) return -1;
		}
		if (k==rdr->ftab.filts[i].nprids)
		{
		//	if (k>(CS_MAXPRFLT-1)) continue;
			if (k>(CS_MAXPRFLT-1))
			{
				mycs_debug(D_ADB, "ftabs:CA{%04X.%08X} CS_MAXPRFLT exceed", casid, prid);
				return -1;
			}
			rdr->ftab.filts[i].prids[k] = prid;
			rdr->ftab.filts[i].nprids++;
			add = 1;
		}
	}
	return (add);
}

static int32_t mavatarcamd_cas_inforeceive(int32_t handle, uint16_t *netMsgId, uint8_t *buffer,
	uint8_t  *deskey, comm_type_t commType,
	uint16_t *pcaid, uint32_t *pprid, uint16_t *pportnum)
{
	int32_t  len, msgid;
	uint8_t  netbuf[CWS_NETMSGSIZE];
	int32_t  returnLen;
	uint16_t casid, portno;
	uint32_t ppid;
	struct s_client *cl = cur_client();

	if (!buffer || handle < 0) return -1;
	len = recv(handle, netbuf, 2, 0);
	cs_log_dbg(D_CLIENT, "mgA(): len=%d, errno=%d", len, (len==-1)  ?errno : 0);
	if (!len) {
		cs_log_dbg(D_CLIENT, "nmr: 1 return 0");
		if (commType == COMMTYPE_CLIENT)
			network_tcp_connection_close(cl->reader, "receive error1");
		else
			cs_disconnect_client(cl);
		return 0;
	}
	if (len != 2) {
		cs_log_dbg(D_CLIENT, "mgA: len!=2");
		if (commType == COMMTYPE_CLIENT)
			network_tcp_connection_close(cl->reader, "receive error2");
		else
			cs_disconnect_client(cl);
		return -1;
	}
	if (((netbuf[0] << 8) | netbuf[1]) > CWS_NETMSGSIZE - 2) {
		cs_log_dbg(D_CLIENT, "mgA: received data len=%d lonage than CWS_NETMSGSIZE=%d", ((netbuf[0] << 8) | netbuf[1]),CWS_NETMSGSIZE);
		cs_log_dbg(D_CLIENT, "mgA: 1 return -1");
		return -1;
	}

	len = recv(handle, netbuf+2, (netbuf[0] << 8) | netbuf[1], 0);
	if (!len) {
		cs_log_dbg(D_CLIENT, "mgA: 2 return 0");
		return 0;
	}
	if (len != ((netbuf[0] << 8) | netbuf[1])) {
		cs_log_dbg(D_CLIENT, "mgA: 2 return -1");
		return -1;
	}
	len += 2;
	if ((len = des_decrypt(netbuf, len, deskey)) < 11) {  // 15(newcamd525) or 11 ???
		cs_log_dbg(D_CLIENT, "mgA: can't decrypt, invalid des key?");
		cs_sleepms(2000);
		return -1;
	}
//	cs_log_dump_dbg(D_CLIENT, netbuf, len, "mgA: decrypted data, len=%d", len);
  	msgid = (netbuf[2] << 8) | netbuf[3];
	returnLen = (((netbuf[13] & 0x0f) << 8) | netbuf[14]) + 3;
	if (returnLen > (len-12)) {
		cs_log_dbg(D_CLIENT, "mgA: 4 return -1");
		return -1;
	}
	if (netbuf[12] == MSG_SERVER_2_CLIENT_OSD) {
		memcpy(cl->reader->ncd_clientmessages, &netbuf[15], netbuf[14]);
		cl->reader->ncd_clientmessages[netbuf[14]+1] = 0;
		myprintf("ncd_clientmessages = %s\n", (char *)cl->reader->ncd_clientmessages);
	}
	portno = b2i(2, &netbuf[4]);
	casid  = b2i(2, &netbuf[6]);
	ppid   = b2i(3, &netbuf[8]);
	mycs_debug(D_CLIENT, "mgA: cas: %04x.%06x(%d)", casid, ppid, portno);
//	cs_log_dump_dbg(D_CLIENT, netbuf, len, "mgA: decrypted data");
	if (netMsgId)
	{
		switch (commType)
		{
			case COMMTYPE_SERVER:
				*netMsgId = msgid;
				break;

			case COMMTYPE_CLIENT:
				if (*netMsgId != ((netbuf[2] << 8) | netbuf[3])) {
					cs_log_dbg(D_CLIENT, "mgA: netMsgId=%d, from server=%d, ", *netMsgId, msgid);
					return -2;
				}
				break;

			default:
				cs_log_dbg(D_CLIENT, "mgA: 5 return -1");
				return -1;
				break;
		}
	}
	switch (commType)
	{
		case COMMTYPE_SERVER:
			memcpy(cl->ncd_header, netbuf, 12);
			buffer[0]=netbuf[4]; // sid
			buffer[1]=netbuf[5];
			break;
		case COMMTYPE_CLIENT:
			memcpy(cl->ncd_header, netbuf, 12);
			buffer[0]=netbuf[2]; // msgid
			buffer[1]=netbuf[3];
			break;
	}
	memcpy(buffer+2, netbuf+12, returnLen);
	if (!portno && !casid && !ppid) return -1;
	*pcaid    = casid;
	*pprid    = ppid;
	*pportnum = portno;
	return returnLen+2;
}
#endif

void newcamd_reply_ka(void)
{
	struct s_client *cl = cur_client();

	if (!cl) return;

	if (!cl->udp_fd) {
		cs_log_dbg(D_CLIENT, "invalid client fd=%d", cl->udp_fd);
		return;
	}

	cs_log_dbg(D_CLIENT, "send keepalive to client fd=%d", cl->udp_fd);

	if (cl->reader)
		cl->reader->last_s = time((time_t *)0);

	network_cmd_no_data_send(cl->udp_fd, &cl->ncd_msgid, MSG_KEEPALIVE, cl->ncd_skey,COMMTYPE_SERVER);
}

static int32_t newcamd_connect_server(void)
{
	int32_t i;
	uint8_t buf[CWS_NETMSGSIZE];
	uint8_t key[16];
	uint8_t keymod[64]; // 14
	int32_t handle=0;
	int32_t keymodsize=14;
	int32_t len=0;

	uint32_t idx;
	uchar passwdcrypt[120];
	uint8_t login_answer;
	int32_t bytes_received;
	struct s_client *cl = cur_client();

	memset(cl->reader->ncd_version,0,sizeof(cl->reader->ncd_version));
	if (cl->reader->device[0] == 0 || cl->reader->r_pwd[0] == 0 ||
		 cl->reader->r_usr [0] == 0 || cl->reader->r_port == 0) {
		return -5;
	}
	// 1. Connect
	handle = network_tcp_connection_open(cl->reader);
	if (handle < 0) return -1;

	// 2. Get init sequence
	cl->ncd_msgid = 0;
#if defined(MODULE_AVAMGCAMD)
	if (IS_AVATARCAMD(cl->reader->ncd_exprotocol)) keymodsize = 64;
#endif

#if defined(WITH_HISILICON)
	i = network_recvpoll(handle, keymod, keymodsize);
	if (i < 0) return -1; // errors during receive!
	if (i != keymodsize) {
		cs_log("server does not return 14 bytes");
		network_tcp_connection_close(cl->reader, "connect error");
		return -2;
	}
#else
	if (read(handle, keymod, keymodsize) != keymodsize) {
		cs_log("server does not return 14 bytes");
		network_tcp_connection_close(cl->reader, "connect error");
		return -2;
	}
#endif

#if defined(MODULE_AVAMGCAMD)
	if (IS_AVATARCAMD(cl->reader->ncd_exprotocol)) {
		keymod[ 0] = keymod[ 5];
		keymod[ 1] = keymod[ 8];
		keymod[ 2] = keymod[14];
		keymod[ 3] = keymod[19];
		keymod[ 4] = keymod[24];
		keymod[ 5] = keymod[26];
		keymod[ 6] = keymod[28];
		keymod[ 7] = keymod[32];
		keymod[ 8] = keymod[34];
		keymod[ 9] = keymod[42];
		keymod[10] = keymod[48];
		keymod[11] = keymod[53];
		keymod[12] = keymod[57];
		keymod[13] = keymod[61];
	}
#endif
	cs_log_dump_dbg(D_CLIENT, keymod, sizeof(cl->reader->ncd_key), "server init sequence:");
	des_login_key_get(keymod, cl->reader->ncd_key, sizeof(cl->reader->ncd_key), key);

	// 3. Send login info
	idx = 3;
	buf[0] = MSG_CLIENT_2_SERVER_LOGIN;
	buf[1] = 0;
	cs_strncpy((char *)buf+idx, cl->reader->r_usr, sizeof(buf)-idx);
	__md5_crypt(cl->reader->r_pwd, "$1$abcdefgh$", (char *)passwdcrypt);
	idx += strlen(cl->reader->r_usr)+1;
	cs_strncpy((char *)buf+idx, (const char *)passwdcrypt, sizeof(buf)-idx);
	len  = idx+strlen((char *)passwdcrypt)+1;
#if defined(MODULE_AVAMGCAMD)
	if (IS_AVATARCAMD(cl->reader->ncd_exprotocol))
	{
		buf[len+0] = 0x76; 	// client ID
		buf[len+1] = 0x45;	// client ID
		buf[len+2] = 0x0;
		buf[len+3] = 0x24;	// device ID
		buf[len+4] = 0x84;	// device ID
		buf[len+5] = 0x0;
		len += 6;
	}
#endif
	network_message_send(handle, 0, buf, len, key,
	               COMMTYPE_CLIENT, NCD_CLIENT_ID, NULL);

	// 3.1 Get login answer
	login_answer = network_cmd_no_data_receive(handle, &cl->ncd_msgid,
	                                   key, COMMTYPE_CLIENT);
	if (login_answer == MSG_CLIENT_2_SERVER_LOGIN_NAK)
	{
		if (cfg.logsvrsecrete) {
			cs_log_sensitive("login failed for user {%s}", cl->reader->r_usr);
		}
		else {
			cs_log("login failed for user %s", cl->reader->r_usr);
		}
		network_tcp_connection_close(cl->reader, "login error1");
		return -3;
	}
	if (login_answer != MSG_CLIENT_2_SERVER_LOGIN_ACK)
	{
		cs_log("expected MSG_CLIENT_2_SERVER_LOGIN_ACK (%02X), received %02X",
		     MSG_CLIENT_2_SERVER_LOGIN_ACK, login_answer);
		network_tcp_connection_close(cl->reader, "login error2");
		return -3;
	}

	// 3.2 Set connection info
	cl->reader->ftab.nfilts = 0;
	cl->reader->tcp_connected = 1;
	cl->crypted = 1;

	// 4. Send MSG_CARD_DATE_REQ
	des_login_key_get(cl->reader->ncd_key, passwdcrypt, strlen((char *)passwdcrypt), key);

	network_cmd_no_data_send(handle, &cl->ncd_msgid, MSG_CARD_DATA_REQ,
	                   key, COMMTYPE_CLIENT);
	bytes_received = network_message_receive(handle, &cl->ncd_msgid, buf,
	                                   key, COMMTYPE_CLIENT);
	if (bytes_received < 16 || buf[2] != MSG_CARD_DATA)
	{
		cs_log("expected MSG_CARD_DATA (%02X), received %02X",
		     MSG_CARD_DATA, buf[2]);
		network_tcp_connection_close(cl->reader, "receive error");
		return -4;
	}
	cs_prdump("MSG_CARD_DATA_REQ", buf, bytes_received);
	// 5. Parse CAID and PROVID(s)
	cl->reader->caid = (uint16_t)((buf[6]<<8) | buf[7]);

	/* handle special serial format in newcamd. See newcamd_auth_client */
	newcamd_to_hexserial(buf+10, cl->reader->hexserial, cl->reader->caid);
	if (cfg.logsvrsecrete) {
		rdr_log_tildes(cl->reader, "%s Server: {%s},{%d} - UserID: %i",
				reader_get_type_desc(cl->reader,1),
				cl->reader->device, cl->reader->r_port, buf[3+2]);
	}
	else {
		rdr_log(cl->reader, "%s Server: %s:%d - UserID: %i",
				reader_get_type_desc(cl->reader,1),
				cl->reader->device, cl->reader->r_port, buf[3+2]);
	}
	rdr_log_sensitive(cl->reader, "CAID: %04X - UA: {%02X%02X%02X%02X%02X%02X%02X%02X} - Provider # %i",
			cl->reader->caid,
			cl->reader->hexserial[0], cl->reader->hexserial[1], cl->reader->hexserial[2], cl->reader->hexserial[3], cl->reader->hexserial[4], cl->reader->hexserial[5], cl->reader->hexserial[6], cl->reader->hexserial[7],
			buf[14+2]);

	cl->reader->nprov = buf[14+2];
	memset(cl->reader->prid, 0x00, sizeof(cl->reader->prid));
	for (i=0; i < cl->reader->nprov; i++)
	{
		if (caid_is_betacrypt(cl->reader->caid) || caid_is_irdeto(cl->reader->caid))
		{
			memcpy(&cl->reader->prid[i], buf+22+2+11*i, 4);
		}
		else
		{
			cl->reader->prid[i][1] = buf[15+2+11*i];
			cl->reader->prid[i][2] = buf[16+2+11*i];
			cl->reader->prid[i][3] = buf[17+2+11*i];
		}
		memcpy(&cl->reader->sa[i], buf+22+2+11*i, 4); // the 4 first bytes are not read
		rdr_log_sensitive(cl->reader, "Provider ID: %02X%02X%02X - SA: {%02X%02X%02X%02X}",
				cl->reader->prid[i][1], cl->reader->prid[i][2], cl->reader->prid[i][3],
				cl->reader->sa[i][0], cl->reader->sa[i][1], cl->reader->sa[i][2], cl->reader->sa[i][3]);
	}

#if defined(MODULE_AVAMGCAMD)
	if (IS_MAVATARCAMD(cl->reader->ncd_exprotocol)) {
		uint16_t casid, portno;
		uint32_t ppid;

		cl->ncd_proto = NCD_525;
		for (i=0; i< 256; i++) {
			bytes_received = mavatarcamd_cas_inforeceive(handle, &cl->ncd_msgid, buf,
	                           key, COMMTYPE_CLIENT,
										&casid, &ppid, &portno);
			if (bytes_received <= 0) break;
			mavatarcamd_cas_addfilters(cl->reader, casid, ppid);
		}
		#if __ADB_TRACE__
			for (i=0; i<cl->reader->ftab.nfilts; i++) {
				if (cl->reader->ftab.filts[i].nprids < 2) {
					myprintf("%3d. CASID: %04X:%06X\n", i, cl->reader->ftab.filts[i].caid, cl->reader->ftab.filts[i].prids[0]);
				}
				else {
					myprintf("%3d. CASID: %04X\n", i, cl->reader->ftab.filts[i].caid);
					int k;
					for (k=0; k<cl->reader->ftab.filts[i].nprids; k++) {
						myprintf("            :%06X\n", cl->reader->ftab.filts[i].prids[k]);
					}
				}
			}
		#endif
	}
#endif

	memcpy(cl->reader->ncd_skey, key, 16);

	// 6. Set card inserted
	cl->reader->tcp_connected = 2;
	cl->reader->card_status   = CARD_INSERTED;
	cl->reader->last_g = cl->reader->last_s = time((time_t *)0);

	// Only after connect() on cl->udp_fd (Linux)
	cl->pfd = cl->udp_fd;

#if defined(MODULE_AVAMGCAMD)
	if (IS_MAVATARCAMD(cl->reader->ncd_exprotocol)) return 0;
#endif

	if (cl->reader->ncd_disable_server_filt) { // act like mgclient
// sky(2016.03.07)
//		network_cmd_no_data_send(handle, &cl->ncd_msgid, MSG_SERVER_2_CLIENT_GET_VERSION,
//	                          key, COMMTYPE_CLIENT);
	}
	return 0;
}

static int32_t newcamd_connect(void)
{
	struct s_client *cl = cur_client();

	if (cl->reader->tcp_connected < 2) {
#if defined(MODULE_AVAMGCAMD)
//		myprintf("xxx=%d, %d\n", cl->reader->ncd_exprotocol, cl->reader->ncd_exregistration);
		if (IS_AVATARCAMD(cl->reader->ncd_exprotocol))
		{
			if (!cs_Isxx(cl->reader->ncd_exkey, 5)) return 0;
			if (!chk_reader_devices(cl->reader)) cl->reader->ncd_exregistration = 0;
			if (!cl->reader->ncd_exregistration) {
				if (!avatarcamd_activation(cl->reader)) return 0;
			}
		}
#endif
		if (newcamd_connect_server() < 0) return 0;
	}
	if (!cl->udp_fd) return 0;
	return 1;
}


static int32_t newcamd_send(uchar *buf, int32_t ml, uint16_t sid)
{
	struct s_client *cl = cur_client();

	if (!newcamd_connect()) return (-1);

	return (network_message_send(cl->udp_fd, &cl->ncd_msgid,
							buf, ml, cl->reader->ncd_skey, COMMTYPE_CLIENT, sid, NULL));
}


static int32_t newcamd_sendto(uchar *buf, int32_t ml, uint16_t sid, custom_data_t *cd)
{
	struct s_client *cl = cur_client();

	if (!newcamd_connect()) return (-1);

	return (network_message_send(cl->udp_fd, &cl->ncd_msgid,
					buf, ml, cl->reader->ncd_skey, COMMTYPE_CLIENT, sid, cd));
}

static int32_t newcamd_recv(struct s_client *client, uchar *buf, int32_t UNUSED(l))
{
	int32_t rc, rs;

	if (client->typ == 'c')
	{
		rs=network_message_receive(client->udp_fd,
               &client->ncd_msgid, buf,
               client->ncd_skey, COMMTYPE_SERVER);
	}
	else
	{
		if (!client->udp_fd) return (-1);
		rs=network_message_receive(client->udp_fd,
               &client->ncd_msgid, buf,
               client->reader->ncd_skey, COMMTYPE_CLIENT);
	}

	if (rs<5) rc=(-1);
	else rc=rs;

	cs_log_dump_dbg(D_CLIENT, buf, rs, "received %d bytes from %s", rs, remote_txt());
	client->last = time((time_t *)0);

	if (rc==-1)
	{
		if (rs > 0)
			cs_log("packet is too small (%d bytes)", rs);
		else
			cs_log("Connection closed to %s", remote_txt());
	}
	return (rc);
}

static void mk_user_au_ftab(struct s_reader *aureader, FILTER *filt)
{
	int32_t i,j,found;
	struct s_client *cl = cur_client();
	FILTER client_filter;
	FILTER *pufilt = &client_filter;

	memset(filt, 0, sizeof(*filt));
	memset(&client_filter, 0, sizeof(client_filter));

	if (cl->ftab.filts) client_filter = cl->ftab.filts[0];

	filt->caid = aureader->caid;
	if (filt->caid == 0)
		filt->caid = client_filter.caid;

	for (i=0; i<aureader->nprov; i++)
		{ filt->prids[filt->nprids++] = b2i(3, &aureader->prid[i][1]); }

	for (i=0; i<pufilt->nprids; i++)
	{
		for(j = found = 0; (!found) && (j < filt->nprids); j++)
			if (pufilt->prids[i] == filt->prids[j]) { found = 1; }
		if (!found)
			{ filt->prids[filt->nprids++] = pufilt->prids[i]; }
	}
}

static void mk_user_ftab(FILTER *filt)
{
	int32_t port_idx,i,j,k,c;
	struct s_client *cl = cur_client();

	memset(filt, 0, sizeof(*filt));

	port_idx = cl->port_idx;
	if (!cfg.ncd_ptab.ports[port_idx].ncd)
		return;
	FILTER *psfilt = &cfg.ncd_ptab.ports[port_idx].ncd->ncd_ftab.filts[0];

	// 1. CAID
	// search server CAID in client CAID
	for(c = i = 0; i < cl->ctab.ctnum; i++)
	{
		CAIDTAB_DATA *d = &cl->ctab.ctdata[i];
		int32_t ctab_caid = d->caid & d->mask;
		if (ctab_caid) { c++; }

		if (psfilt->caid==ctab_caid)
		{
			filt->caid = ctab_caid;
			break;
		}
	}
	if (c && !filt->caid)
	{
		cs_log("no valid CAID found in CAID for user '%s'", cl->account->usr);
		return;
	}

	// search CAID in client IDENT
	cs_log_dbg(D_CLIENT, "client[%8lX].%s nfilts=%d, filt.caid=%04X", (unsigned long)pthread_self(),
				  cl->account->usr, cl->ftab.nfilts, filt->caid);

	if (!filt->caid && cl->ftab.filts)
	{
		int32_t fcaids;
		for (i=fcaids=0; i<cl->ftab.nfilts; i++)
		{
			uint16_t ucaid=cl->ftab.filts[i].caid;
			if (ucaid) { fcaids++; }
			if (ucaid && psfilt->caid==ucaid)
			{
				filt->caid = ucaid;
				break;
			}
		}
		if (fcaids == cl->ftab.nfilts && !filt->caid)
		{
			cs_log("no valid CAID found in IDENT for user '%s'", cl->account->usr);
		//	cs_disconnect_client();
			return;
		}
	}
	// empty client CAID - use server CAID
	if (!filt->caid) { filt->caid = psfilt->caid; }

	// 2. PROVID
	if (!cl->ftab.nfilts)
	{
		int32_t add;
		for (i=0; i<psfilt->nprids; i++)
		{
			// use server PROVID(s) (and only those which are in user's groups)
			add = 0;
			struct s_reader *rdr;
			for (rdr=first_active_reader; rdr; rdr=rdr->next)
			if (rdr->grp & cl->grp)
			{
				if (!rdr->ftab.nfilts)
				{
					if (is_network_reader(rdr)) { add = 1; }
					for (j=0; !add && j<rdr->nprov; j++)
						if (b2i(3, &rdr->prid[j][1]) == psfilt->prids[i]) { add = 1; }
				}
				else
				{
					for(j = 0; !add && j < rdr->ftab.nfilts; j++)
					{
						uint32_t rcaid = rdr->ftab.filts[j].caid;
						if (!rcaid || rcaid == filt->caid)
						{
							for (k=0; !add && k<rdr->ftab.filts[j].nprids; k++)
									if (rdr->ftab.filts[j].prids[k] == psfilt->prids[i]) { add = 1; }
						}
					}
				}
			}
			if (add) { filt->prids[filt->nprids++] = psfilt->prids[i]; }
		}
		memcpy(filt, psfilt, sizeof(*filt));
		return;
	}

	// search in client IDENT
	for (j=0; j<cl->ftab.nfilts; j++)
	{
		uint32_t ucaid = cl->ftab.filts[j].caid;
		cs_log_dbg(D_CLIENT, "client caid %d: %04X", j, ucaid);
		if (!ucaid || ucaid == filt->caid)
		{
			for (i=0; i<psfilt->nprids; i++)
			{
				cs_log_dbg(D_CLIENT, "search server provid %d: %06X", i, psfilt->prids[i]);
				if (cl->ftab.filts[j].nprids)
				{
					for (k=0; k<cl->ftab.filts[j].nprids; k++)
						if (cl->ftab.filts[j].prids[k] == psfilt->prids[i])
							{ filt->prids[filt->nprids++] = cl->ftab.filts[j].prids[k]; }
				}
				else
				{
					filt->prids[filt->nprids++] = psfilt->prids[i];
				// allow server PROVID(s) if no PROVID(s) specified in IDENT
				}
			}
		}
	}

	if (!filt->nprids)
	{
		cs_log("no valid PROVID(s) found in CAID for user '%s'", cl->account->usr);
	//	cs_disconnect_client();
	}
}

static int8_t newcamd_auth_client(IN_ADDR_T ip, uint8_t *deskey)
{
	int32_t i, ok, rc, sid_list;
	uchar *usr = NULL, *pwd = NULL;
	struct s_auth *account;
	uchar buf[14];
	uchar key[16];
	uchar passwdcrypt[120];
	struct s_reader *aureader = NULL, *rdr = NULL;
	struct s_client *cl = cur_client();
	uchar mbuf[CWS_NETMSGSIZE];

	sid_list = 0;

	ok = cfg.ncd_allowed ? check_ip(cfg.ncd_allowed, ip) : 1;

	if (!ok)
	{
		cs_auth_client(cl, (struct s_auth *)0, NULL);
		return -1;
	}

	// make random 14 bytes
	get_random_bytes(buf, 14);

	// send init sequence
	send(cl->udp_fd, buf, 14, 0);
	des_login_key_get(buf, deskey, 14, key);
	memcpy(cl->ncd_skey, key, 16);
	cl->ncd_msgid = 0;

	i=process_input(mbuf, sizeof(mbuf), cfg.cmaxidle);
	if (i>0)
	{
		if (mbuf[2] != MSG_CLIENT_2_SERVER_LOGIN)
		{
			cs_log_dbg(D_CLIENT, "expected MSG_CLIENT_2_SERVER_LOGIN (%02X), received %02X",
			MSG_CLIENT_2_SERVER_LOGIN, mbuf[2]);
			return -1;
		}
		usr=mbuf+5;
		pwd=usr+strlen((char *)usr)+1;
	}
	else
	{
		cs_log_dbg(D_CLIENT, "bad client login request");
		return -1;
	}

	cl->ncd_client_id = (mbuf[0] << 8) | mbuf[1];
	const char *client_name = newcamd_get_client_name(cl->ncd_client_id);
#if defined(TCP_KEEPIDLE)
	if (cl->ncd_client_id == 0x4453) // DiabloWifi has problems with TCPKeepAlive
	{
		int32_t flag = 600;
		if (setsockopt(cl->udp_fd, IPPROTO_TCP, TCP_KEEPIDLE, &flag, sizeof(flag)) && errno != EBADF) {	//send first keepalive packet after 600 seconds of last package received (keepalive packets included)
			cs_log("Setting TCP_KEEPIDLE failed, errno=%d, %s", errno, strerror(errno));
		}
		else {
			cs_log("WARNING: Setting TCP_KEEPIDLE to 10 minutes for bugged DiabloWifi. Note that this might lead to not detected broken connections or multiple connections.");
		}
	}
#endif

	if (cl->ncd_proto==NCD_525 && 0x6D == mbuf[0]
		&& 0x67 == mbuf[1] && 0x11 == cl->ncd_header[11])
	{
		sid_list = 1;
	}

	for (ok=0, account=cfg.account; (usr) && (account) && (!ok); account=account->next)
	{
		cs_log_dbg(D_CLIENT, "account->usr=%s", account->usr);
		if (strcmp((char *)usr, account->usr) == 0)
		{
			__md5_crypt(ESTR(account->pwd), "$1$abcdefgh$", (char *)passwdcrypt);
			cs_log_dbg(D_CLIENT, "account->pwd=%s", passwdcrypt);
			if (strcmp((char *)pwd, (const char *)passwdcrypt) == 0)
			{
				cl->crypted=1;
				char e_txt[20];
				snprintf(e_txt, 20, "%s:%d", "newcamd", cfg.ncd_ptab.ports[cl->port_idx].s_port);
				if ((rc = cs_auth_client(cl, account, e_txt)) == 2) {
					cs_log("hostname or ip mismatch for user %s (%s)", usr, client_name);
					break;
				}
				else if (rc != 0)
				{
					cs_log("account is invalid for user %s (%s)", usr, client_name);
					break;
				}
				else
				{
					cs_log("user %s authenticated successfully (%s)", usr, client_name);
					ok = 1;
					break;
				}
			}
			else {
				cs_log("user %s is providing a wrong password (%s)", usr, client_name);
			}
		}
	}

	if (!ok && !account)
	{
		cs_log("user %s is trying to connect but doesnt exist ! (%s)", usr, client_name);
		usr = 0;
	}

	// check for non ready reader and reject client
	for (rdr=first_active_reader; rdr; rdr=rdr->next) {
		if (!cfg.ncd_ptab.ports[cl->port_idx].ncd)
			continue;
		if (rdr->caid==cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts[0].caid) {
			if (rdr->card_status == CARD_NEED_INIT) {
				cs_log("init for reader %s not finished -> reject client", rdr->label);
				ok = 0;
			}
			break;
		}
	}

	if (ok)
	{
		LL_ITER itr = ll_iter_create(cl->aureader_list);
		while ((rdr = ll_iter_next(&itr)))
		{
			int32_t n;
			if (!cfg.ncd_ptab.ports[cl->port_idx].ncd)
				continue;
			for (n=0;n<cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts[0].nprids;n++)
			{
				if (emm_reader_match(rdr, cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts[0].caid, cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts[0].prids[n])) {
					aureader = rdr;
					break;
				}
			}
			if (aureader)
				break;
		}

		if (aureader) {
			cs_log("AU enabled for user %s on reader %s", usr, aureader->label);
		}
		else {
			cs_log("AU disabled for user %s", usr);
		}
	}

	network_cmd_no_data_send(cl->udp_fd, &cl->ncd_msgid,
	          (ok) ? MSG_CLIENT_2_SERVER_LOGIN_ACK : MSG_CLIENT_2_SERVER_LOGIN_NAK,
	          cl->ncd_skey, COMMTYPE_SERVER);

	if (ok)
	{
		FILTER usr_filter;
		FILTER *pufilt = &usr_filter;

		des_login_key_get(deskey, passwdcrypt, strlen((char *)passwdcrypt), key);
		memcpy(cl->ncd_skey, key, 16);

		i=process_input(mbuf, sizeof(mbuf), cfg.cmaxidle);
		if (i>0)
		{
			int32_t j,len=15;
			if (mbuf[2] != MSG_CARD_DATA_REQ)
			{
				cs_log_dbg(D_CLIENT, "expected MSG_CARD_DATA_REQ (%02X), received %02X",
				       MSG_CARD_DATA_REQ, mbuf[2]);
				return -1;
			}

			// set userfilter for au enabled clients
			if (aureader)
				mk_user_au_ftab(aureader, &usr_filter);
			else
				mk_user_ftab(&usr_filter);

			if (!cfg.ncd_mgclient)
				ftab_add(&cl->ftab, &usr_filter); //We cannot filter all cards!

			mbuf[0] = MSG_CARD_DATA;
			mbuf[1] = 0x00;
			mbuf[2] = 0x00;

			if (aureader)
				{ mbuf[3] = 1; }
			else
				{ mbuf[3] = get_threadnum(cl) + 10; } // Unique user number

			mbuf[4] = (uchar)(pufilt->caid>>8);
			mbuf[5] = (uchar)(pufilt->caid);
			mbuf[6] = 0x00;
			mbuf[7] = 0x00;

			if (aureader)
				{ hexserial_to_newcamd(aureader->hexserial, mbuf + 8, pufilt->caid); }
			else
				{ memset(&mbuf[8], 0, 6); } //mbuf[8] - mbuf[13]

			mbuf[14] = pufilt->nprids;
			for (j=0; j<pufilt->nprids; j++)
			{
				if (caid_is_betacrypt(pufilt->caid) || caid_is_irdeto(pufilt->caid))
				{
					mbuf[15+11*j] = 0;
					mbuf[16+11*j] = 0;
					mbuf[17+11*j] = j;
				}
				else
				{
					mbuf[15+11*j] = (uchar)(pufilt->prids[j]>>16);
					mbuf[16+11*j] = (uchar)(pufilt->prids[j]>>8);
					mbuf[17+11*j] = (uchar)(pufilt->prids[j]);
				}
				mbuf[18+11*j] = 0x00;
				mbuf[19+11*j] = 0x00;
				mbuf[20+11*j] = 0x00;
				mbuf[21+11*j] = 0x00;
				if (aureader)
				{
					// check if user provid from IDENT exists on card
					int32_t k, found;
					uint32_t rprid;
					found=0;
						if (pufilt->caid==aureader->caid)
						{
							for (k=0; (k<aureader->nprov); k++)
							{
								rprid=b2i(3, &aureader->prid[k][1]);
								if (rprid==pufilt->prids[j])
								{
									if (caid_is_betacrypt(pufilt->caid) || caid_is_irdeto(pufilt->caid))
									{
										mbuf[22+11*j] = aureader->prid[k][0];
										mbuf[23+11*j] = aureader->prid[k][1];
										mbuf[24+11*j] = aureader->prid[k][2];
										mbuf[25+11*j] = aureader->prid[k][3];
									}
									else
									{
										mbuf[22+11*j] = aureader->sa[k][0];
										mbuf[23+11*j] = aureader->sa[k][1];
										mbuf[24+11*j] = aureader->sa[k][2];
										mbuf[25+11*j] = aureader->sa[k][3];
									}
									found=1;
									break;
								}
							}
						}
						if (!found)
						{
							mbuf[22+11*j] = 0x00;
							mbuf[23+11*j] = 0x00;
							mbuf[24+11*j] = 0x00;
							mbuf[25+11*j] = 0x00;
						}
				}
				else
				{
					if (caid_is_betacrypt(pufilt->caid) || caid_is_irdeto(pufilt->caid))
					{
						mbuf[22+11*j] = 0x00;
						mbuf[23+11*j] = (uchar)(pufilt->prids[j]>>16);
						mbuf[24+11*j] = (uchar)(pufilt->prids[j]>>8);
						mbuf[25+11*j] = (uchar)(pufilt->prids[j]);
					}
					else
					{
						mbuf[22+11*j] = 0x00;
						mbuf[23+11*j] = 0x00;
						mbuf[24+11*j] = 0x00;
						mbuf[25+11*j] = 0x00;
					}
				}
				len+=11;
			}

			custom_data_t cd;
			memset(&cd, 0, sizeof(cd));

			if (aureader)
			{
				if (aureader->blockemm & EMM_GLOBAL && !(aureader->saveemm & EMM_GLOBAL))
					{ cd.sid |= 4; }
				if (aureader->blockemm & EMM_SHARED && !(aureader->saveemm & EMM_SHARED))
					{ cd.sid |= 2; }
				if (aureader->blockemm & EMM_UNIQUE && !(aureader->saveemm & EMM_UNIQUE))
					{ cd.sid |= 1; }
			}

			if (network_message_send(cl->udp_fd, &cl->ncd_msgid,
							mbuf, len, key, COMMTYPE_SERVER, 0, &cd) <0)
			{
				return -1;
			}
		}

		// send SID list
		if (sid_list)
			{ send_sid_list(); }
	}
	else
	{
		cs_auth_client(cl, 0, usr ? "login failure" : "no such user");
		return -1;
	}
	return 0;
}

static void newcamd_send_dcw(struct s_client *client, ECM_REQUEST *er)
{
	int32_t len;
	uint16_t cl_msgid;
	uchar mbuf[19];

	if (!client->udp_fd) {
		cs_log_dbg(D_CLIENT, "ncd_send_dcw: error: client->udp_fd=%d", client->udp_fd);
		return;
	}

	cl_msgid = er->msgid;
	mbuf[0]  = er->ecm[0];
	if ((client->ftab.filts && client->ftab.filts[0].nprids == 0) || (er->rc >= E_NOTFOUND /*not found*/))
	{
		len = 3;
		mbuf[1] = mbuf[2] = 0x00;
	}
	else
	{
		len = 19;
		mbuf[1] = mbuf[2] = 0x10;
		memcpy(mbuf+3, er->cw, 16);
	}

	cs_log_dbg(D_CLIENT, "ncd_send_dcw: er->msgid=%d, cl_msgid=%d, %02X", er->msgid, cl_msgid, mbuf[0]);

	network_message_send(client->udp_fd, &cl_msgid, mbuf, len,
	                   client->ncd_skey, COMMTYPE_SERVER, 0, NULL);
}

static void newcamd_process_ecm(struct s_client *cl, uchar *buf, int32_t len)
{
	int32_t pi;
	ECM_REQUEST *er;

	if (!(er = get_ecmtask())) {
	  	return;
	}
	// save client ncd_msgid
	er->msgid = cl->ncd_msgid;
	er->ecmlen= (((buf[3] & 0x0F) << 8) | buf[4]) + 3;
	cs_log_dbg(D_CLIENT, "ncd_process_ecm: er->msgid=%d len=%d ecmlen=%d", er->msgid, len, er->ecmlen);
	er->srvid = cl->ncd_header[4]<< 8 | cl->ncd_header[5];
	er->caid  = cl->ncd_header[6]<< 8 | cl->ncd_header[7];
	er->prid  = cl->ncd_header[8]<<16 | cl->ncd_header[9]<<8 | cl->ncd_header[10];
	if (!er->caid) {
		pi = cl->port_idx;
		if (cfg.ncd_ptab.nports && cfg.ncd_ptab.nports >= pi && cfg.ncd_ptab.ports[pi].ncd)
		  	er->caid=cfg.ncd_ptab.ports[pi].ncd->ncd_ftab.filts[0].caid;
	}
	memcpy(er->ecm, buf+2, er->ecmlen);
	get_cw(cl, er);
}

static void newcamd_process_emm(uchar *buf)
{
	int32_t  ok=1;
	uint16_t caid = 0;
	struct s_client *cl = cur_client();
	EMM_PACKET epg;

	memset(&epg, 0, sizeof(epg));

	epg.emmlen = buf[2]+3;
	caid = cfg.ncd_ptab.ports[cl->port_idx].ncd->ncd_ftab.filts[0].caid;
	epg.caid[0] = (uchar)(caid>>8);
	epg.caid[1] = (uchar)(caid);

/*
	epg.provid[0] = (uchar)(aureader->auprovid>>24);
	epg.provid[1] = (uchar)(aureader->auprovid>>16);
	epg.provid[2] = (uchar)(aureader->auprovid>>8);
	epg.provid[3] = (uchar)(aureader->auprovid);
*/
/*	if (caid == 0x0500)
  	{
		uint16_t emm_head;

		emm_head = (buf[0]<<8) | buf[1];
		switch (emm_head)
		{
			case 0x8e70:  // EMM-S
				memcpy(epg.hexserial+1, buf+3, 4);
				epg.hexserial[4] = aureader->hexserial[4];
				break;
			case 0x8870:  // EMM-U
			case 0x8c70:  // confidential ?
			default:
				cs_log("unsupported emm type: %04X", emm_head);
				ok=0;
		}
		if (!ok) cs_log("only EMM-S supported");
  	}
	else
*/
	memcpy(epg.emm, buf, epg.emmlen);
	if (ok)
		do_emm(cl, &epg);

	// Should always send an answer to client (also if au is disabled),
	// some clients will disconnect if they get no answer
	buf[1] = 0x10;
   buf[2] = 0x00;
	network_message_send(cl->udp_fd, &cl->ncd_msgid, buf, 3,
	                   cl->ncd_skey, COMMTYPE_SERVER, 0, NULL);
}

static void newcamd_report_cards(struct s_client *client)
{
	int32_t j, k, l;
	uint8_t buf[512];
	custom_data_t *cd;
	if (!cs_malloc(&cd, sizeof(struct custom_data)))
		return;
	memset(buf, 0, sizeof(buf));

	cd->sid = cfg.ncd_ptab.ports[client->port_idx].s_port;

	buf[0]  = MSG_SERVER_2_CLIENT_ADDCARD;
	struct s_reader *rdr;
	for (rdr=first_active_reader; rdr; rdr=rdr->next)
	{
		int32_t flt = 0;
		if (!(rdr->grp & client->grp)) continue; //test - skip unaccesible readers
		if (rdr->ftab.filts)
		{
			for(j = 0; j < rdr->ftab.nfilts; j++)
			{
				if (rdr->ftab.filts[j].caid)
				{
					cd->caid = rdr->ftab.filts[j].caid;
					if (!rdr->ftab.filts[j].nprids) {
						cd->provid = 0;
						cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X svc",cd->caid, cd->provid);
						network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
					}
					for (k=0; k<rdr->ftab.filts[j].nprids; k++) {
						cd->provid = rdr->ftab.filts[j].prids[k];
						cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X svc",cd->caid, cd->provid);
						network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
						flt = 1;
					}
				}
			}
		}

		if (rdr->caid && !flt)
		{
			if ((rdr->tcp_connected || rdr->card_status == CARD_INSERTED))
			{
				cd->caid = rdr->caid;
				if (!rdr->nprov)
				{
					cd->provid = 0;
					cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X caid",cd->caid, cd->provid);
					network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
				}
				for (j=0; j<rdr->nprov; j++)
				{
					cd->provid = (rdr->prid[j][1]) << 16 | (rdr->prid[j][2] << 8) | rdr->prid[j][3];
					cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X caid",cd->caid, cd->provid);
					network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
				}
			}
		}
	}
	if (cfg.sidtab && client->account)
	{
		struct s_sidtab *ptr;
		for (j=0,ptr=cfg.sidtab; ptr; ptr=ptr->next,j++)
		{
	    	if (client->account->sidtabs.ok&((SIDTABBITS)1<<j))
            for (k=0;k<ptr->num_caid;k++)
            {
            	cd->caid = ptr->caid[k];
            	if (!ptr->num_provid)
            	{
                	cd->provid = 0;
                	cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X acs",cd->caid, cd->provid);
                	network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
            	}
      			for (l=0;l<ptr->num_provid;l++)
      			{
                	cd->provid = ptr->provid[l];
                	cs_log_dbg(D_CLIENT, "newcamd: extended: report card %04X:%06X acs",cd->caid, cd->provid);
                	network_message_send(client->udp_fd, &client->ncd_msgid, buf, 3, client->ncd_skey, COMMTYPE_SERVER, 0, cd);
        			}
            }
	    }
	}
	NULLFREE(cd);

}

static void newcamd_server_init(struct s_client *client)
{
	int8_t res = 0;

	client->ncd_server = 1;
	cs_log("client connected to %d port", cfg.ncd_ptab.ports[client->port_idx].s_port);

	if (cfg.ncd_ptab.ports[client->port_idx].ncd && cfg.ncd_ptab.ports[client->port_idx].ncd->ncd_key_is_set) {
		//port has a des key specified
		res = newcamd_auth_client(client->ip, cfg.ncd_ptab.ports[client->port_idx].ncd->ncd_key);
	}
	else {
		//default global des key
		res = newcamd_auth_client(client->ip, cfg.ncd_key);
	}

	if (res == -1) {
		cs_disconnect_client(client);
		return;
	}

	// report all cards if using extended mg proto
	if (cfg.ncd_mgclient) {
		cs_log_dbg(D_CLIENT, "newcamd: extended: report all available cards");
		newcamd_report_cards(client);
	}

}

#define EXT_VERSION_STR "1.67"
#define EXT_VERSION_LEN 4

static void newcamd_send_version(struct s_client *client)
{
	uchar buf[30];
	memset(buf, 0, sizeof(buf));
	buf[0] = MSG_SERVER_2_CLIENT_GET_VERSION;
	buf[1] = EXT_VERSION_LEN >>8;
	buf[2] = EXT_VERSION_LEN & 0xFF;
	memcpy(buf+3, EXT_VERSION_STR, EXT_VERSION_LEN);
	network_message_send(client->udp_fd, &client->ncd_msgid, buf, EXT_VERSION_LEN+3, client->ncd_skey, COMMTYPE_SERVER, 0, NULL);
}

static void *newcamd_server(struct s_client *client, uchar *mbuf, int32_t len)
{
	// check for clienttimeout, if timeout occurs try to send keepalive / wait for answer
	// befor client was disconnected. If keepalive was disabled, exit after clienttimeout

	if (len<3)
		return NULL;

	cs_log_dbg(D_CLIENT, "newcamd: got cmd %d", mbuf[2]);

	switch (mbuf[2]) {
		case 0x80:
		case 0x81:
			newcamd_process_ecm(client, mbuf, len);
			break;

		case MSG_SERVER_2_CLIENT_GET_VERSION:
			cs_log_dbg(D_CLIENT, "newcamd: extended: send Version 1.67");
			newcamd_send_version(client);
			break;

		case MSG_KEEPALIVE:
			newcamd_reply_ka();
			break;

		default:
			if (mbuf[2]>0x81 && mbuf[2]<0x90)
				newcamd_process_emm(mbuf+2);
			else {
				cs_log_dbg(D_CLIENT, "unknown newcamd command! (%d)", mbuf[2]);
			}
	}

	return NULL;
}

void newcamd_idle(void)
{
	struct s_client *client = cur_client();
	struct s_reader *rdr = client->reader;

	if (!rdr) return;

	if (rdr->tcp_ito > 0) {
		// inactivitytimeout > 0 enables protocol keepalive packages
		time_t  now;
		int32_t time_diff;
		time(&now);
		time_diff = llabs(now - rdr->last_s);
		if (time_diff>(rdr->tcp_ito)) {
			if (client->ncd_keepalive)
				newcamd_reply_ka();
			else
				network_tcp_connection_close(client->reader, "inactivity");
		}
	}
	else if (rdr->tcp_ito == -1) {
		// idle reconnect
		newcamd_connect();
	}
}

/*
* client functions
*/

int32_t newcamd_client_init(struct s_client *client)
{

	char ptxt[1] = { "\0" };

	client->ncd_proto = client->reader->ncd_proto;

	if (cfg.logsvrsecrete) {
		cs_log_sensitive("proxy {%s},{%d} newcamd52%d (fd=%d%s)",
		       client->reader->device, client->reader->r_port,
		      (client->reader->ncd_proto==NCD_525)?5:4, client->udp_fd, ptxt);
	}
	else {
		cs_log("proxy %s,%d newcamd52%d (fd=%d%s)",
		       client->reader->device, client->reader->r_port,
		      (client->reader->ncd_proto==NCD_525)?5:4, client->udp_fd, ptxt);
	}
	cs_log("  newcamd:%d,%d", client->reader->ncd_connect_on_init, client->reader->tcp_ito);
	// try to connect. ignore possible failures
	// idle reconnect (tcp_ito = -1) will trigger an additional connect anyway
	if (client->reader->ncd_connect_on_init && client->reader->tcp_ito != -1)
		newcamd_connect();

	return (0);
}

static int32_t newcamd_send_ecm(struct s_client *client, ECM_REQUEST *er, uchar *buf)
{
	struct s_reader *rdr = client->reader;
	int returnval;

  	if (!newcamd_connect()) return (-1);

	// check server filters
	if (!chk_rsfilter(rdr, er)) return (-2);

	memcpy(buf, er->ecm, er->ecmlen);

	client->ncd_header[ 4] = er->srvid >> 8;
	client->ncd_header[ 5] = er->srvid &  0xFF;
	client->ncd_header[ 6] = er->caid  >> 8;
	client->ncd_header[ 7] = er->caid  &  0xFF;
	client->ncd_header[ 8] = er->prid  >> 16;
	client->ncd_header[ 9] = er->prid  >> 8;
	client->ncd_header[10] = er->prid  &  0xFF;
#if defined(MODULE_AVAMGCAMD)
	if (IS_MAVATARCAMD(rdr->ncd_exprotocol)) {
		custom_data_t cd;
		cd.sid    = er->srvid;
		cd.caid   = er->caid;
		cd.provid = er->exprid;
		returnval = newcamd_sendto(buf, er->ecmlen, er->srvid, &cd);
	}
	else
#endif
	{
		returnval = newcamd_send(buf, er->ecmlen, er->srvid);
	}
	return ((returnval < 1) ? (-1) : 0);
}


static int32_t newcamd_send_emm(EMM_PACKET *ep)
{
	uchar buf[ep->emmlen];

	if (!newcamd_connect())
		return (-1);

	memcpy(buf, ep->emm, ep->emmlen);
	return ((newcamd_send(buf, ep->emmlen, 0) < 1) ? 0 : 1);
}

static int32_t newcamd_recv_chk(struct s_client *client, uchar *dcw, int32_t *rc, uchar *buf, int32_t n)
{
	uint16_t idx = -1;
	int lenn;

	if (n<5)
		return -1;

	switch (buf[2]) {
		case 0x80:
		case 0x81:
		  	idx = (buf[0] << 8) | buf[1];
		  	if (n==5) { //not found on server
		    	*rc = 0;
		    	memset(dcw, 0, 16);
		    	break;
		  	}

		  	if (n<21) {
		    	cs_log_dbg(D_CLIENT, "invalid newcamd cw answer");
		    	return (-1);
		  	}

		  	*rc = 1;
		  	memcpy(dcw, buf+5, 16);
		  	break;

		case MSG_KEEPALIVE:
		  	return -1;

		case MSG_SERVER_2_CLIENT_ADDCARD:
			if (client->reader) {
				client->reader->ncd_disable_server_filt = 1;
			}
			return -1;
		// sky()
		case MSG_SERVER_2_CLIENT_GET_VERSION: // 0xD6
			lenn = (buf[3]<<8) | buf[4];
			if (lenn>EXT_VERSION_LEN) lenn = EXT_VERSION_LEN;
			strncpy(client->reader->ncd_version, (char *)&buf[5], lenn);
			client->reader->ncd_version[lenn] = 0;
		  	cs_log_dbg(D_CLIENT, "newcamd version %s from server", client->reader->ncd_version);
		  	return -1;

		case MSG_SERVER_2_CLIENT_OSD:
#if defined(MODULE_AVAMGCAMD)
			if (IS_MAVATARCAMD(client->reader->ncd_exprotocol)) {
				memcpy(client->reader->ncd_clientmessages, &buf[5], buf[4]);
				client->reader->ncd_clientmessages[buf[4]+1] = 0;
				myprintf("ncd_clientmessages = %s\n", (char *)client->reader->ncd_clientmessages);
			}
#endif
			return -1; // 0xD1

		default:
		  	if (buf[2]>0x81 && buf[2]<0x90) { // answer to emm
		    	return -1;
		  	}
		  	cs_log_dbg(D_CLIENT, "unknown newcamd command{%02X} from server", buf[2]);
		  	return -1;
	}
	return (idx);
}


// sky(n)
int32_t newcamd_cAvailable(struct s_reader *rdr, int32_t checktype, ECM_REQUEST *er)
{
	if (!er)  return 0;
	if (!rdr) return 0;
	if (checktype == AVAIL_CHECK_CONNECTED) {
//		#if defined(MODULE_AVAMGCAMD)
//		if (IS_MAVATARCAMD(rdr->ncd_exprotocol)) return 1;
//		#endif
//		if (rdr->ncd_connect_on_init) {
//			if (!rdr->ncd_disable_server_filt) {
//				if (!chk_cafilters(rdr, er)) return 0;
//			}
//		}
	}
	return 1;
}

#if defined(MODULE_AVAMGCAMD)
//	#define	__AVATAR_SIMULATION__
#ifdef __AVATAR_SIMULATION__
static uint8_t avatar_simulation_message[0x186] = {
							0xe3,0xee,0xe7,0xfd,0xe0,0xf7,0xf5,0xbb,0xa1,0xfa,0xa1,0xb8,0xa1,0xfc,0xa1,0xfa,
							0xa1,0xb0,0xb6,0xb0,0xb8,0xb8,0xa1,0xfc,0xa1,0xfa,0xa1,0xed,0xe0,0xf2,0xf5,0xe5,
							0xf3,0xe4,0xe0,0xec,0xaf,0xe0,0xf7,0xe0,0xf5,0xe0,0xf3,0xe2,0xe0,0xec,0xe9,0xe5,
							0xaf,0xe2,0xee,0xec,0xa1,0xfc,0xa1,0xfa,0xa1,0xeb,0xee,0xef,0xe4,0xe4,0xf5,0xe4,
							0xf2,0xf5,0xb1,0xb1,0xb1,0xb1,0xb4,0xa1,0xfc,0xa1,0xfa,0xa1,0xb9,0xb4,0xb7,0xb7,
							0xb7,0xb6,0xb5,0xb2,0xb5,0xb0,0xa1,0xfc,0xa1,0xfa,0xa1,0xb0,0xb0,0xa1,0xb1,0xb3,
							0xa1,0xb0,0xb2,0xa1,0xb1,0xb5,0xa1,0xb0,0xb4,0xa1,0xb1,0xb7,0xa1,0xb0,0xb6,0xa1,
							0xb1,0xb9,0xa1,0xb0,0xb8,0xa1,0xb0,0xb1,0xa1,0xb3,0xb0,0xa1,0xb0,0xb3,0xa1,0xb3,
							0xb2,0xa1,0xb0,0xb5,0xa1,0xfc,0xa1,0xfa,0xa1,0xb1,0xa1,0xfc,0xfd,0xe7,0xf5,0xf1,
							0xbb,0xa1,0xfa,0xa1,0xb0,0xa1,0xfc,0xa1,0xfa,0xa1,0xb3,0xb0,0xa1,0xfc,0xa1,0xfa,
							0xa1,0xe7,0xf5,0xf1,0xaf,0xe2,0xee,0xec,0xe8,0xf9,0xac,0xe5,0xf7,0xe3,0xaf,0xe2,
							0xee,0xec,0xa1,0xfc,0xa1,0xa1,0xfa,0xa1,0xe0,0xef,0xee,0xef,0xf8,0xec,0xee,0xf4,
							0xf2,0xa1,0xfc,0xa1,0xfa,0xa1,0xf4,0xf2,0xe4,0xf3,0xc1,0xe9,0xee,0xf2,0xf5,0xa1,
							0xfc,0xa1,0xfa,0xa1,0xe2,0xee,0xec,0xe8,0xf9,0xa1,0xfc,0xa1,0xfa,0xa1,0xb0,0xa1,
							0xfc,0xfd,0xf6,0xe9,0xe5,0xbb,0xfa,0xa1,0xb0,0xa1,0xfc,0xa1,0xfa,0xa1,0xb1,0xa1,
							0xfc,0xa1,0xfa,0xa1,0xec,0xe4,0xe5,0xe8,0xe0,0xaf,0xe2,0xee,0xec,0xe8,0xf9,0xac,
							0xe5,0xf7,0xe3,0xaf,0xe2,0xee,0xec,0xae,0xe2,0xee,0xec,0xe8,0xf9,0xa1,0xfc,0xa1,
							0xfa,0xa1,0xe2,0xee,0xec,0xe8,0xf9,0xa1,0xfc,0xa1,0xfa,0xa1,0xe2,0xee,0xec,0xe8,
							0xf9,0xb8,0xb8,0xb8,0xe9,0xe5,0xa1,0xfc,0xa1,0xfa,0xa1,0xb1,0xb1,0xa1,0xfc,0xa1,
							0xfa,0xa1,0xb0,0xa1,0xfc,0xfd,0xe0,0xe2,0xf5,0xbb,0xfa,0xa1,0xb0,0xa1,0xfc,0xa1,
							0xfa,0xa1,0xb0,0xb3,0xb4,0xb0,0xb3,0xa1,0xfc,0xa1,0xfa,0xa1,0xe0,0xe2,0xf5,0xe8,
							0xf7,0xe4,0xaf,0xe2,0xee,0xec,0xe8,0xf9,0xac,0xe5,0xf7,0xe3,0xaf,0xe2,0xee,0xec,
							0xa1,0xfc,0xa1,0xfa,0xa1,0xb1,0xb1,0xa1,0xfc,0xa1,0xfa,0xa1,0xb1,0xb1,0xa1,0xfc,
							0xa1,0xfa,0xa1,0xb1,0xb1,0xa1,0xfc,0xa1,0xfa,0xa1,0xb0,0xa1,0xfc,0xfd,0xe4,0xee,
							0xe7,0xfd,0xe4,0xee,0xf2,0x0d,
};
#endif

static int32_t
avatarcamd_register_parser(struct s_reader *reader, char *messages, int msgsize)
{
	int  start, end;
	int  i, j,k;
	int  Avtidx = 0;
	int  Avtfound = 0;
	int  Avtcompleted = 0;

	start = end = 0;
	for (i=0; i<msgsize; i++)
	{
		/* "avt:", "ftp:", "whd:", "act:" ....*/
		if (!strncmp(&messages[i],"avt:",4))
		{
			myprintf("avtarcamd:FIND Avt packet.\n");
			Avtfound = 1;
		}
		else if (messages[i]=='{')
		{
			start = i;
		}
		else if (messages[i]=='}')
		{
			end = i;
			Avtidx++;
			myascdump("avtarcamd:parse", &messages[start], end-start+1);
			switch (Avtidx)
			{
				case 1:
					break;
				case 2:
					if (!Avtfound) break;
					reader->r_port = cs_a2i(&messages[start+2], end-start-1);
					myprintf("avtarcamd:server Port : %d\n", reader->r_port);
					break;
				case 3:
					if (!Avtfound) break;
					memcpy(reader->device, &messages[start+2], end-start-1);
					reader->device[end-(start+3)] = 0;
					myprintf("avtarcamd:server hostname : %s\n", reader->device);
					break;
				case 4:
					if (!Avtfound) break;
					memcpy(reader->r_usr, &messages[start+2], end-start-1);
					reader->r_usr[end-(start+3)] = 0;
					myprintf("avtarcamd:server username : %s\n", reader->r_usr);
					break;
				case 5:
					if (!Avtfound) break;
					memcpy(reader->r_pwd, &messages[start+2], end-start-1);
					reader->r_pwd[end-(start+3)] = 0;
					myprintf("avtarcamd:server password : %s\n", reader->r_pwd);
					break;
				case 6:
					if (!Avtfound) break;
					for(k=0,j=start+2; j<end; j+=3, k++){
						cs_atob(&reader->ncd_key[k], &messages[j], 1);
					}
					Avtcompleted = 1;
					myprdump("avtarcamd:server DESKEY : ", reader->ncd_key, 14);
					break;
				default:
					break;

			}
			if (Avtcompleted) break;
		}
	}
	return Avtfound;
}

static int32_t
avatarcamd_registration(struct s_reader *reader, int32_t sockfd)
{
	uint32_t	ip = 0;
	uint8_t 	macaddress[6];
	uint8_t	securityid[32];
	char 		activecode[256];
	uint8_t	recvbuf[512];
	int 		lenn,i;
	int 		returncode = 0;

//	Read_SD_ID(securityid); put own your device ID.
	memset(securityid,0,32);
	memset(macaddress, 0, sizeof(macaddress));
	network_chk_intefaces(&ip, macaddress);
	sprintf(activecode,
				"%02x%02x%02x%02x%02x|absm%02x%02x%02x%02x%02x%02x%02x%02x|%02x%02x%02x%02x%02x%02x|C5",
				reader->ncd_exkey[0],
				reader->ncd_exkey[1],
				reader->ncd_exkey[2],
				reader->ncd_exkey[3],
				reader->ncd_exkey[4],
				securityid[0],securityid[1],securityid[2],securityid[3],securityid[4],securityid[5],securityid[6],securityid[7],
				macaddress[0],macaddress[1],macaddress[2],macaddress[3],macaddress[4],macaddress[5]);
	myprintf("avtarcamd:Avart KEY : %s\n", activecode);
	for(i=0; i<(int)strlen(activecode); i++) {
		activecode[i] = activecode[i]^0x81;
	}

	if ((lenn = send(sockfd, activecode, sizeof(activecode),0)) < 0) {
		myprintf("avatarcamd:register send failure\n");
		return -1;
	}
	lenn = network_recvpoll(sockfd, recvbuf, sizeof(recvbuf)-1);
	if (lenn <= 0) {
		myprintf("avatarcamd:register recv failure\n");
		return -1;
	}
	#ifdef __AVATAR_SIMULATION__
		lenn = sizeof(avatar_simulation_message);
		memcpy(recvbuf, avatar_simulation_message, sizeof(avatar_simulation_message));
	#endif
//	myprdump("RANDOM DATA", recvbuf, lenn);
	for (i=0; i<lenn; i++) {
		recvbuf[i] = recvbuf[i]^0x81;
	}
	recvbuf[lenn] = 0;
	myascdump("RANDOM decode", recvbuf, lenn);
	returncode = avatarcamd_register_parser(reader, (char *)recvbuf, lenn);
	if (returncode==0) {
		mycs_trace(D_TRACE, "avatarcamd:activation failure");
		return -1;
	}
	return returncode;
}

static int32_t
avatarcamd_activation(struct s_reader *reader)
{
	const char register_server[32] = "hybrid.crombocode.com";
	int32_t register_port = 12512;
	int32_t sockfd;
	int32_t registered = 0;

	mycs_trace(D_TRACE, "avatarcamd:avatarcamd_activation");
	sockfd = network_tcp_socket_open(reader, (char *)register_server, register_port);
	if (sockfd <= 0) return 0;
	registered = avatarcamd_registration(reader, sockfd);
	network_tcp_socket_close(reader, sockfd);
	if (registered < 1) {
		reader->card_status = CARD_UNREGISTER;
		return 0;
	}
	reader->ncd_exregistration = 1;
	write_server();
	return 1;
}
#endif

/*
 * resolve client type for newcamd protocol
 */
const char *newcamd_get_client_name(uint16_t client_id)
{
	// When adding new entries keep the list sorted!
	static const struct {
		uint16_t id;
		const char *client;
	} ncd_service_ids[] = {
		{ 0x0000, "generic" },
		{ 0x02C2, "Opticum" },
		{ 0x0665, "rq-sssp-client-CS" },
		{ 0x0666, "rqcamd" },
		{ 0x0667, "rq-echo-client" },
		{ 0x0669, "rq-sssp-client-CW" },
		{ 0x0769, "JlsRq" },
		{ 0x414C, "AlexCS" },
		{ 0x4333, "camd3" },
		{ 0x4343, "CCcam" },
		{ 0x434C, "Cardlink" },
		{ 0x4453, "DiabloCam-UW" },
		{ 0x4543, "eyetvCamd" },
		{ 0x4765, "Octagon" },
		{ 0x4C43, "LCE" },
		{ 0x4E58, "NextYE2k" },
		{ 0x5342, "SBCL" },
		{ 0x5456, "Tecview" },
		{ 0x5644, "vdr-sc" },
		{ 0x5743, "WiCard" },
		{ 0x6378, "cx" },
		{ 0x6502, "Tvheadend" },
		{ 0x6576, "evocamd" },
		{ 0x6762, "gbox2CS" },
		{ 0x6B61, "Kaffeine" },
		{ 0x6B63, "kpcs" },
		{ 0x6D63, "mpcs" },
		{ 0x6D67, "mgcamd" },
		{ 0x7645, "avatarcamd" },
		{ 0x6E65, "NextYE2k" },
		{ 0x6E73, "NewCS" },
		{ 0x7264, "radegast" },
		{ 0x7363, "Scam" },
		{ 0x7763, "WinCSC" },
		{ 0x7878, "tsdecrypt" },
		{ 0x8888, "OSCam" },
		{ 0x9911, "ACamd" },
		{ 0x9922, "DVBplug" },
		{ 0xFFFF, NULL }
	};
	int i = 0;
	while(1)
	{
		if (!ncd_service_ids[i].client)
			{ break; }
		if (ncd_service_ids[i].id == client_id)
			{ return ncd_service_ids[i].client; }
		i++;
	}
	return "unknown - please report";
}

void module_newcamd(struct s_module *ph)
{
	ph->desc = "newcamd";
	ph->type = MOD_CONN_TCP;
  	ph->large_ecm_support = 1;
	ph->listenertype = LIS_NEWCAMD;
	IP_ASSIGN(ph->s_ip, cfg.ncd_srvip);
	ph->s_handler = newcamd_server;
	ph->s_init = newcamd_server_init;
	ph->recv = newcamd_recv;
	ph->send_dcw = newcamd_send_dcw;
	ph->ptab = cfg.ncd_ptab;
	ph->c_init = newcamd_client_init;
	ph->c_recv_chk = newcamd_recv_chk;
	ph->c_send_ecm = newcamd_send_ecm;
	ph->c_send_emm = newcamd_send_emm;
	ph->c_idle = newcamd_idle;
	// sky(n)
	ph->c_available = newcamd_cAvailable;
	ph->num=R_NEWCAMD;
}
#endif
