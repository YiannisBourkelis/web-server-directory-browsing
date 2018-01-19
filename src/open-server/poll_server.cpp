/* ***********************************************************************
 * (C) 2018 by Yiannis Bourkelis (hello@andama.org)
 * ***********************************************************************/

// Using poll() instead of select()
// poll server based on code from https://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/rzab6/poll.htm

// SSL Programming Tutorial
// http://h41379.www4.hpe.com/doc/83final/ba554_90007/ch04s03.html

// Simple TLS Server
// https://wiki.openssl.org/index.php/Simple_TLS_Server

// an echo server using the libev library
// https://sourceforge.net/p/libevent-thread/code/ci/master/tree/echoserver_threaded.c#l265

// Thread Pools in NGINX Boost Performance 9x!
// https://www.nginx.com/blog/thread-pools-boost-performance-9x/

// epoll example
// https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/

// scalable network io in linux
// http://www.citi.umich.edu/techreports/reports/citi-tr-00-4.pdf


#include "poll_server.h"
#include "helper_functions.h"
#include "html_messageprocessor.h"

#include "qglobal.h" //for Q_UNUSED

int PollServer::s_listen_sd;

PollServer::PollServer()
{
}

void PollServer::InitializeSSL()
{
    /*
    SSL_load_error_strings();
    SSL_library_init(); //xreiazetai gia to linux logo bug se palaioteres ekdoseis openssl
    OpenSSL_add_all_algorithms();
    */
}

void PollServer::DestroySSL()
{
    /*
    ERR_free_strings();
    EVP_cleanup();
    */
}

void PollServer::create_context()
{
    /*
    const SSL_METHOD *method;

    method = TLSv1_2_server_method();

    sslctx_ = SSL_CTX_new(method);
    if (!sslctx_) {
    perror("Unable to create SSL context");
    exit(EXIT_FAILURE);
    }
    */
}

void PollServer::configure_context()
{
    /*
    SSL_CTX_set_ecdh_auto(sslctx_, 1);

    // Set the key and cert
    if (SSL_CTX_use_certificate_file(sslctx_, "../certificate.pem", SSL_FILETYPE_PEM) <= 0) {
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(sslctx_, "../key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        exit(EXIT_FAILURE);
    }
    */
}

void PollServer::displayLastError(std::string description){
    Q_UNUSED(description);
#ifdef MY_DEBUG
#ifdef WIN32
        std::cout <<  description << " - Last error number: " << WSAGetLastError() << std::endl;
#else
        perror(description.data());
#endif
#endif
}

#ifdef WIN32
void PollServer::disableNagleAlgorithm(SOCKET socket){
#else
void PollServer::disableNagleAlgorithm(int socket){
#endif
    /*************************************************************/
    /* SIMANTIKO: kanw disable to nagle algorithm. meiwnei to latency. */
    /*************************************************************/
   int flag = 1;
   int setsockopt_nagle_ret = 0;
   setsockopt_nagle_ret = setsockopt(socket,                    /* socket affected */
                           IPPROTO_TCP,     /* set option at TCP level */
                           TCP_NODELAY,     /* name of option */
                           (char *) &flag,  /* the cast is historical cruft */
                           sizeof(int));    /* length of option value */
   if (setsockopt_nagle_ret < 0){
       displayLastError("setsockopt to disable nagle algorithm failed for listening socket");
   }
}

#ifdef WIN32
void PollServer::setSocketNonBlocking(SOCKET socket){
#else
void PollServer::setSocketNonBlocking(int socket){
#endif
    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for    */
    /* the incoming connections will also be nonblocking since  */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    int rc, on = 1;
    #ifdef WIN32
    rc = ioctlsocket(socket, FIONBIO, (u_long*)&on);
    #else
    rc = ioctl(socket, FIONBIO, (char *)&on);
    #endif
    if (rc < 0)
    {
      displayLastError("ioctl() failed for listen_sd");
      #ifdef WIN32
      closesocket(socket);
      #else
      close(socket);
      #endif
      exit(-1);
    }
}

void PollServer::checkClosedSessions()
{
    {
        std::lock_guard<std::mutex> qlk(HTML_MessageProcessor::qclients_close_mutex);

        if (HTML_MessageProcessor::qclients_close.size() > 0){
            for (auto& c:HTML_MessageProcessor::qclients_close){
                if (fds[c.fds_index].fd == c.socket) {
                    qwe("will close socket: ", c.socket);
                    close(c.socket);
                    fds[c.fds_index].fd = -1;
                    fds[c.fds_index].revents = 0;
                    fds[c.fds_index].events = 0;
                }
            }//for
            HTML_MessageProcessor::qclients_close.clear();
        }
    }
}

void PollServer::start(int server_port, protocol ip_protocol)
{
  Q_UNUSED(ip_protocol);
  int    len, rc, on = 1;
  int    listen_sd = -1, new_sd = -1;
  bool   end_server = false, compress_array = false;
  int    close_conn;
  //char   buffer[80];
  std::vector<char> recv_buffer(16192);
  struct sockaddr_in   addr;
  int    nfds = 1, current_size = 0, i, j;

  /*************************************************************/
  /* Create an AF_INET stream socket to receive incoming       */
  /* connections on                                            */
  /*************************************************************/
  listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0)
  {
    perror("socket() failed");
    exit(-1);
  }

  /*************************************************************/
  /* Allow socket descriptor to be reuseable                   */
  /*************************************************************/
  rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                  (char *)&on, sizeof(on));
  if (rc < 0)
  {
    perror("setsockopt() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for    */
  /* the incoming connections will also be nonblocking since  */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/
  rc = ioctl(listen_sd, FIONBIO, (char *)&on);
  if (rc < 0)
  {
    perror("ioctl() failed");
    close(listen_sd);
    exit(-1);
  }

  disableNagleAlgorithm(listen_sd);

  /*************************************************************/
  /* Bind the socket                                           */
  /*************************************************************/
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(server_port);
  rc = bind(listen_sd,
            (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0)
  {
    perror("bind() failed");
    close(listen_sd);
    exit(-1);
  }

  /*************************************************************/
  /* Set the listen back log                                   */
  /*************************************************************/
  rc = listen(listen_sd, 32);
  if (rc < 0)
  {
    perror("listen() failed");
    close(listen_sd);
    exit(-1);
  }

  PollServer::s_listen_sd = listen_sd;

  /*************************************************************/
  /* Initialize the pollfd structure                           */
  /*************************************************************/
  memset(fds, 0 , sizeof(fds));

  /*************************************************************/
  /* Set up the initial listening socket                        */
  /*************************************************************/
  fds[0].fd = listen_sd;
  fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;


  /*************************************************************/
  /* Loop waiting for incoming connects or for incoming data   */
  /* on any of the connected sockets.                          */
  /*************************************************************/
  qwe("Entering poll() loop...", "");
  do
  {
    /***********************************************************/
    /* Call poll() and wait 3 minutes for it to complete.      */
    /***********************************************************/
    qwe("Waiting on poll()...", "");
    rc = poll(fds, nfds, -1);

    checkClosedSessions();

    /***********************************************************/
    /* Check to see if the poll call failed.                   */
    /***********************************************************/
    if (rc < 0)
    {
      perror("  poll() failed");
      qwe("error no: ", errno);

      if (errno == EINVAL){
          compress_array = true;
      }
      //continue;
    }

    /***********************************************************/
    /* Check to see if the 3 minute time out expired.          */
    /***********************************************************/
    if (rc == 0)
    {
      continue;
      //printf("  poll() timed out.  End program.\n");
      //break;
    }


    /***********************************************************/
    /* One or more descriptors are readable.  Need to          */
    /* determine which ones they are.                          */
    /***********************************************************/
    current_size = nfds;
    for (i = 0; i < current_size; i++)
    {
      /*********************************************************/
      /* Loop through to find the descriptors that returned    */
      /* POLLIN and determine whether it's the listening       */
      /* or the active connection.                             */
      /*********************************************************/
      if(fds[i].revents == 0)
        continue;

      /*********************************************************/
      /* If revents is not POLLIN, it's an unexpected result,  */
      /* log and end the server.                               */
      /*********************************************************/
      if(!(fds[i].revents & POLLIN))
      {
        qwe("Error! revents = ", fds[i].revents);
        qwe("will close socket: ", fds[i].fd);
        if (fds[i].fd > 0){
            close(fds[i].fd);
            fds[i].fd = -1;
        }
        compress_array = true;
        fds[i].revents = 0;
        continue;
        //end_server = true;
        //break;

      }

      if (fds[i].fd == listen_sd)
      {
        /*******************************************************/
        /* Listening descriptor is readable.                   */
        /*******************************************************/
        qwe("Listening socket is readable\n", "");

        /*******************************************************/
        /* Accept all incoming connections that are            */
        /* queued up on the listening socket before we         */
        /* loop back and call poll again.                      */
        /*******************************************************/
        do
        {
          /*****************************************************/
          /* Accept each incoming connection. If              */
          /* accept fails with EWOULDBLOCK, then we            */
          /* have accepted all of them. Any other             */
          /* failure on accept will cause us to end the        */
          /* server.                                           */
          /*****************************************************/
          new_sd = accept(listen_sd, NULL, NULL);
          if (new_sd < 0)
          {
            if (errno != EWOULDBLOCK || errno != EAGAIN)
            {
              perror("  accept() failed");
              //end_server = true;
            }
            break;
          }

          if (new_sd == 0){
              //simvainei kai afto den kserw giati
              qwe ("new socket = 0 !!!", "");
              break;
          }

          disableNagleAlgorithm(new_sd);
          setSocketNonBlocking(new_sd);

          /*****************************************************/
          /* Add the new incoming connection to the            */
          /* pollfd structure                                  */
          /*****************************************************/
          qwe("New incoming connection: ", new_sd);
          fds[nfds].fd = new_sd;
          fds[nfds].events = POLLIN;
          nfds++;

          /*****************************************************/
          /* Loop back up and accept another incoming          */
          /* connection                                        */
          /*****************************************************/
        } while (new_sd != -1); //do ACCEPT connections loop
      } // if listen_sd




      /*********************************************************/
      /* This is not the listening socket, therefore an        */
      /* existing connection must be readable                  */
      /*********************************************************/
      else
      {
        qwe("Descriptor is readable: ", fds[i].fd);
        close_conn = false;
        /*******************************************************/
        /* Receive all incoming data on this socket            */
        /* before we loop back and call poll again.            */
        /*******************************************************/

        do
        {
          /*****************************************************/
          /* Receive data on this connection until the         */
          /* recv fails with EWOULDBLOCK. If any other        */
          /* failure occurs, we will close the                 */
          /* connection.                                       */
          /*****************************************************/
          if (!use_ssl_){
            rc = recv(fds[i].fd, recv_buffer.data(), recv_buffer.size(), 0);
          } else {
             //SSL_set_fd(sslmap_.at(fds[i].fd), fds[i].fd);
             //rc = SSL_read(sslmap_.at(fds[i].fd), recv_buffer.data(), recv_buffer.size());
          }

          if (rc < 0)
          {
            if (errno != EWOULDBLOCK || errno != EAGAIN)
            {
              perror("  recv() failed");
              close_conn = true;
            }
            break;
          }

          /*****************************************************/
          /* Check to see if the connection has been           */
          /* closed by the client                              */
          /*****************************************************/
          if (rc == 0)
          {
            qwe("Connection closed", "");
            close_conn = true;
            break;
          }

          /*****************************************************/
          /* Data was received                                 */
          /*****************************************************/
          len = rc;
          qwe("`Bytes received: ", len);

          msgComposer->onClientDataArrived(fds[i].fd, i, recv_buffer, rc);

        } while(true);

        /*******************************************************/
        /* If the close_conn flag was turned on, we need       */
        /* to clean up this active connection. This           */
        /* clean up process includes removing the              */
        /* descriptor.                                         */
        /*******************************************************/
        if (close_conn)
        {
          qwe("will close socket: ", fds[i].fd);
          if (fds[i].fd > 0) {
            close(fds[i].fd);
          }
          fds[i].fd = -1;
          compress_array = true;
        }


      }  /* End of existing connection is readable             */
    } /* End of Forloop through pollable descriptors              */

    /***********************************************************/
    /* If the compress_array flag was turned on, we need       */
    /* to squeeze together the array and decrement the number  */
    /* of file descriptors. We do not need to move back the    */
    /* events and revents fields because the events will always*/
    /* be POLLIN in this case, and revents is output.          */
    /***********************************************************/
    if (compress_array)
    {
      compress_array = false;
      for (i = 0; i < nfds; i++)
      {
        if (fds[i].fd == -1)
        {
          for(j = i; j < nfds; j++)
          {
            fds[j].fd = fds[j+1].fd;
            fds[j].events = fds[j+1].events;
            fds[j].revents = fds[j+1].revents;
          }
          nfds--;
        }
      }
    }

  } while (end_server == false); /* End of serving running.    */

  /*************************************************************/
  /* Clean up all of the sockets that are open                  */
  /*************************************************************/
  for (i = 0; i < nfds; i++)
  {
    if(fds[i].fd >= 0)
      close(fds[i].fd);
  }
}
