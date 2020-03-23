#ifndef HEADER_NET_CONTROLLER_TCPCOMMON
#define HEADER_NET_CONTROLLER_TCPCOMMON

namespace hudp {

#define TCP_MSS_DEFAULT		 536U	/* IPv4 (RFC1122, RFC2581) */
#define MAX_PACING_RATE      (uint32_t)(~0U)
enum tcp_ca_state {
        TCP_CA_Open = 0,
#define TCPF_CA_Open	(1<<TCP_CA_Open)
        TCP_CA_Disorder = 1,
#define TCPF_CA_Disorder (1<<TCP_CA_Disorder)
        TCP_CA_CWR = 2,
#define TCPF_CA_CWR	(1<<TCP_CA_CWR)
        TCP_CA_Recovery = 3,
#define TCPF_CA_Recovery (1<<TCP_CA_Recovery)
        TCP_CA_Loss = 4
#define TCPF_CA_Loss	(1<<TCP_CA_Loss)
};

}

#endif