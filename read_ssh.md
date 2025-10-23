# rfc 4521: ssh architecture

ssh consists of:
- transport layer protocol: server authentication, confidentiality, integrity and optionally compression
- user authentication protocol: user to server auth
- connection protocol: multiplexes tunnel (e.g. tcp link) to logical tunnels

## host key
used for user to identify server
- user maintains hostname-hostkey database
- client only knows CA root key that certifies host

## data type
- byte, boolean, uint32, uint64, string, 
- mpint: multi-precision int, uint32+string, MSB first
- name-list: uint32+string

## message numbers
- 1-19 transport layer
- 20-29 alg nego
- 30-49 kex
- 50-59 user auth
- 60-79 user auth method
- 80-89 connection protocol
- 90-127 channel message
- 192-255 extension 

# binary packet format

- uint32 packet-length
- byte padding-length
- byte[ n1 ] payload
- byte[ n2 ] random padding
- byte[ m ] mac

where packet-length = 1+n1+n2 < 32768 (Bytes), padding-length = n2, mac = MAC(key, seq_number|unencrypted-packet)

attributes: compression(none, zlib), cipher(...), mac(...)



# connection setup

## protocol version exchange(4253)
< -- > SSH-2.0-< sw ver >(\s< comments >)?\r\n
## kex(4253,...)
alg negotiation: < -- > kexinit
```
kexinit{
    byte16 cookie;
    name-list kex-alg;
    name-list server-host-key-alg; // server side signing at the end of kex
    name-list cipher-c2s-alg;
    name-list cipher-s2c-alg;
    name-list mac-c2s-alg;
    name-list mac-s2c-alg;
    name-list lang-c2s;
    name-list lang-s2c;
    bool first_kex_follows; // de facto =0
    u32 reserved; // MBZ
}
```

using ECDH-type kex(5656, integration to ssh; 7748, ecdh alg):

server < -- kex_ecdh_init
```
kex_ecdh_init{
    string Q_c; // client ephemeral public key
}
```
server -- > kex_ecdh_reply
```
kex_ecdh_reply{
    string K_s; // server host key
    string Q_s; // server ephemeral public key
    string sig; // signature on exchange hash
}
```
where exchange hash H = HASH(V_c|V_s|I_c|I_s|K_s|Q_c|Q_s|K)
; then compute iv_c2s = HASH(K|H|'A'|session_id),..., cipher-key and mac-key as follows
; the expansion of key complies K2=HASH(K|H|K1), K=K1|K2...

server < -- > newkeys

on failure: server -- > disconnect{u32 reason, string desc, string lang}

> encryption(cipher) starts after "newkeys"

## user auth(4252)

server < -- service_request{service-name="ssh-userauth"}
server -- > service_accept{service-name}

(optional)server < -- userauth_request(dryrun)
```
userauth_request{
    string user-name;
    string service-name; // service to start after auth
    method-name; // publickey|password|hostbased|none
    *kargs;
}
```
(optional)server -- > userauth_..._ok

server < -- userauth_request

server -- > userauth_success

on failure: userauth_failure{name-list, bool}; warning: userauh_banner{string banner, lang}

publickey args: {bool not_dryrun; string alg; string blob;}; password args: {bool not_dryrun=FALSE; string passwd;}; 

## openning channel(session, 4254)



# algorithms