# /media/black/Data/lib/libiscsi/bin/iscsi-ls iscsi://127.0.0.1:3260
./utils/iscsi-ls iscsi://127.0.0.1/
# Target:iqn.2023-05.black.com:server Portal:127.0.0.1:3260,1
# Target:iqn.2003-01.org.linux-iscsi.black.x8664:sn.dbf8950fa890 Portal:127.0.0.1:3260,1



# Persistent Reservation, PR
./utils/iscsi-pr



#0  iscsi_process_pdu (iscsi=0x55555555d6e0, in=0x55555555fa50) at pdu.c:543
#1  0x00007ffff7f7870e in iscsi_read_from_socket (iscsi=0x55555555d6e0) at socket.c:739
#2  0x00007ffff7f794b0 in iscsi_tcp_service (iscsi=0x55555555d6e0, revents=1) at socket.c:1028
#3  0x00007ffff7f7956b in iscsi_service (iscsi=0x55555555d6e0, revents=1) at socket.c:1052
#4  0x0000555555558032 in main (argc=1, argv=0x7fffffffd5f8) at iscsiclient.c:677
