//No server usamos a solução sugerida pelo professor que é o uso de um ficheiro com o número de pedidos total

void *CommAgent(void *arg){
    printf("CommAgent Started\n");
    int Udp_Server_Socket_fd = (AF_INET, SOCK_DGRAM, 0); //Descriptor da socket UDP, AF_INET especifica que vai trabalhar sobre IpV4
    dados_CommAgent *dados = (dados_CommAgent *)arg;
    char server_message[2000], client_message[2000];
    struct sockaddr_in server; //Esta socket é mais por questões de facilitar a vida ao usar o inet
    char buffer_numpedidos[11];
    buffer_numpedidos[11] = '\0';
    int fd_rqfile = open("Num_Requests.txt",O_RDWR);
    read(fd_rqfile,buffer_numpedidos,sizeof(buffer_numpedidos)); //Podiamos optar por uma solução mais bonita a partir do fstat

    int  num_pedidos = atoi(buffer_numpedidos); 
    printf("Ip: %s,", dados->IP_add);
    printf("%lc\n",sizeof(dados->IP_add));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(dados->IP_add);
    server.sin_port = htons(dados->port);

    if(Udp_Server_Socket_fd < 0){
        printf("UDP socket creation wasn't possible, shutting down!");
        exit(-1); //Podemos trocar por um exit;
    } //Erro na Criação

    //Basicamente dizer que a socket passa a ter estas propriedades
    while(bind(Udp_Server_Socket_fd, (struct sockaddr*)&server, sizeof(server)) < 0){
        //É PRECISO FAZER UM TIPO DE TIMEOUT AQUI SENÃO SE REALMENTE ISTO ESTIVER A DAR MAL FICAMOS EM UM LOOP INFINITO
    }

    //Verificamos se a socket ficou atribuída
    int len_nome_server = sizeof(server);
    socklen_t length_server = len_nome_server;
    if(getsockname(Udp_Server_Socket_fd,&server,len_nome_server)){
        printf("Port failed to be trully attributed to the specified parameters");
        exit(-3);
    }

}