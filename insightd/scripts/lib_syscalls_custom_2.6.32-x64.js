// additional syscall information, generated by hand from manpages
//
// created by diekmann
// 2011-06-16


//recvfrom: added argument names
SysCalls[45] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'sockfd',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': True,
               'isUser': True,
               'name': 'buf',
               'type': 'void',
               'user_ptr': '__user *'},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'len',
               'type': 'size_t',
               'user_ptr': ''},
      'arg3': {'isPtr': False,
               'isUser': False,
               'name': 'flags',
               'type': 'unsigned',
               'user_ptr': ''},
      'arg4': {'isPtr': True,
               'isUser': True,
               'name': 'src_addr',
               'type': 'struct sockaddr',
               'user_ptr': '__user *'},
      'arg5': {'isPtr': True,
               'isUser': True,
               'name': 'addrlen',
               'type': 'int',
               'user_ptr': '__user *'},
      'argc': 6,
      'signature': 'int, void __user *, size_t, unsigned, struct sockaddr __user *, int __user *',
      'sys_name': 'sys_recvfrom',
      'sys_real_name': 'sys_recvfrom'};

//changed naming convention to manpage
SysCalls[46] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'sockfd',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': True,
               'isUser': True,
               'name': 'msg',
               'type': 'struct msghdr',
               'user_ptr': '__user *'},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'flags',
               'type': 'unsigned',
               'user_ptr': ''},
      'argc': 3,
      'signature': 'int fd, struct msghdr __user *msg, unsigned flags',
      'sys_name': 'sys_sendmsg',
      'sys_real_name': 'sys_sendmsg'}


SysCalls[44] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'sockfd',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': True,
               'isUser': True,
               'name': 'buf',
               'type': 'void',
               'user_ptr': '__user *'},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'len',
               'type': 'size_t',
               'user_ptr': ''},
      'arg3': {'isPtr': False,
               'isUser': False,
               'name': 'flags',
               'type': 'unsigned',
               'user_ptr': ''},
      'arg4': {'isPtr': True,
               'isUser': True,
               'name': 'dest_addr',
               'type': 'struct sockaddr',
               'user_ptr': '__user *'},
      'arg5': {'isPtr': False,
               'isUser': False,
               'name': 'addrlen',
               'type': 'int',
               'user_ptr': ''},
      'argc': 6,
      'signature': 'int sockfd, void __user *buf, size_t len, unsigned flags, struct sockaddr __user * dest_addr, int addrlen',
      'sys_name': 'sys_sendto',
      'sys_real_name': 'sys_sendto'}


SysCalls[49] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'sockfd',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': True,
               'isUser': True,
               'name': 'addr',
               'type': 'struct sockaddr',
               'user_ptr': '__user *'},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'addrlen',
               'type': 'int',
               'user_ptr': ''},
      'argc': 3,
      'signature': 'int sockfd, struct sockaddr __user *addr, int addrlen',
      'sys_name': 'sys_bind',
      'sys_real_name': 'sys_bind'}
      
SysCalls[41] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'domain',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': False,
               'isUser': False,
               'name': 'type',
               'type': 'int',
               'user_ptr': ''},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'protocol',
               'type': 'int',
               'user_ptr': ''},
      'argc': 3,
      'signature': 'int domain, int type, int protocol',
      'sys_name': 'sys_socket',
      'sys_real_name': 'sys_socket'}

SysCalls[42] = {'arg0': {'isPtr': False,
               'isUser': False,
               'name': 'sockfd',
               'type': 'int',
               'user_ptr': ''},
      'arg1': {'isPtr': True,
               'isUser': True,
               'name': 'addr',
               'type': 'struct sockaddr',
               'user_ptr': '__user *'},
      'arg2': {'isPtr': False,
               'isUser': False,
               'name': 'addrlen',
               'type': 'int',
               'user_ptr': ''},
      'argc': 3,
      'signature': 'int sockfd, struct sockaddr __user * addr, int addrlen',
      'sys_name': 'sys_connect',
      'sys_real_name': 'sys_connect'}
