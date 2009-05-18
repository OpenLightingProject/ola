/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * network.c
 * Network code for libshownet
 * Copyright (C) 2005-2006 Simon Newton
 */

//once again, order is an issue here
#include "private.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>


typedef struct iface_s {
  struct sockaddr_in ip_addr;
  struct sockaddr_in bcast_addr;
  int8_t hw_addr[SHOWNET_MAC_LENGTH];
  char   if_name[32];  // 32 sounds a reasonable size
  struct iface_s *next;
} iface_t;

#ifdef HAVE_GETIFADDRS
 #ifdef HAVE_LINUX_IF_PACKET_H
   #define USE_GETIFADDRS
 #endif
#endif

#ifdef USE_GETIFADDRS
  #include <ifaddrs.h>
    #include <linux/types.h>
  #include <linux/if_packet.h>
#endif

#define INITIAL_IFACE_COUNT 10
#define IFACE_COUNT_INC 5

/*
 * free memory used by the iface's list
 *
 * @param head  pointer to the head of the list
 */
static void free_ifaces(iface_t *head) {
  iface_t *ift , *ift_next;

  for (ift = head; ift != NULL; ift = ift_next) {
    ift_next = ift->next;
    free(ift);
  }
}

#ifdef USE_GETIFADDRS


/*
 * check if we are interested in this interface
 *
 * @param ifa  pointer to a ifaddr struct
 */
static iface_t *check_iface(struct ifaddrs *ifa) {
  struct sockaddr_in *sin;
  iface_t *ret = 0;

  if (!ifa || !ifa->ifa_addr) return 0;

  // skip down, loopback and non inet interfaces
  if (!(ifa->ifa_flags & IFF_UP)) return 0;
  if (ifa->ifa_flags & IFF_LOOPBACK) return 0;
  if (ifa->ifa_addr->sa_family != AF_INET) return 0;

  ret = calloc(1, sizeof(iface_t));
  if(ret == NULL) {
    shownet_error("%s : calloc error %s" , __FUNCTION__, strerror(errno) );
    return 0;;
  }

  sin = (struct sockaddr_in *) ifa->ifa_addr;
  ret->ip_addr.sin_addr = sin->sin_addr;
  strncpy(ret->if_name, ifa->ifa_name, 31);

  if (ifa->ifa_flags & IFF_BROADCAST) {
    sin = (struct sockaddr_in *) ifa->ifa_broadaddr;
    ret->bcast_addr.sin_addr = sin->sin_addr;
  }

  return ret;
}


/*
 * Returns a linked list of interfaces on this machine using getifaddrs
 *  loopback interfaces are skipped as well as interfaces which are down
 *
 * @param ift_head_r address of the pointer to the head of the list
 */
static int get_ifaces(iface_t **ift_head_r) {
  struct ifaddrs *ifa, *ifa_iter;
  iface_t *if_head, *if_tmp, *if_iter;
  struct sockaddr_ll *sll;
  char *if_name, *cptr;

  ifa_iter = 0;
  if( getifaddrs(&ifa) != 0) {
    shownet_error("Error getting interfaces: %s", strerror(errno));
    return SHOWNET_ENET;
  }

  if_head = 0;
  if_iter = 0;
  for(ifa_iter = ifa; ifa_iter != NULL; ifa_iter = ifa_iter->ifa_next) {

    sll = (struct sockaddr_ll *) ifa_iter->ifa_addr;

    // check if AF_INET, up and non-loopback
    if_tmp = check_iface(ifa_iter);
    if(if_tmp) {
      // We got new usable interface
      if(!if_iter) {
        if_head = if_iter =  if_tmp;
      } else {
        if_iter->next = if_tmp;
        if_iter = if_tmp;
      }
    }
  }

  // Match up the interfaces with the corrosponding AF_PACKET interface
  // to fetch the hw addresses
  //
  // XXX: Will probably not work on OS X, it should
  //      return AF_LINK -type sockaddr
  for(if_iter = if_head; if_iter!=NULL; if_iter = if_iter->next) {
    if_name = strdup(if_iter->if_name);

    // if this is an alias, get mac of real interface
    if ( ( cptr = strchr(if_name, ':') ))
      *cptr = 0;

    // Find corresponding iface_t -structure
    for(ifa_iter = ifa; ifa_iter != NULL; ifa_iter = ifa_iter->ifa_next) {
      if((! ifa_iter->ifa_addr) || ifa_iter->ifa_addr->sa_family  != AF_PACKET)
        continue;

      if(strncmp(if_name, ifa_iter->ifa_name, 32)==0) {
        // Found matching hw-address
        sll = (struct sockaddr_ll *) ifa_iter->ifa_addr;
        memcpy(if_iter->hw_addr , sll->sll_addr, 6);
        break;
      }
    }
    free(if_name);
  }

  *ift_head_r = if_head;
  freeifaddrs(ifa);
  return 0;
}

#else


/*
 *
 * Returns a linked list of interfaces on this machine using ioctl's
 *  loopback interfaces are skipped as well as interfaces which are down
 *
 * @param ift_head_r address of the pointer to the head of the list
 */
static int get_ifaces(iface_t **ift_head_r) {
  struct ifconf ifc;
  struct ifreq *ifr, ifrcopy;
  struct sockaddr_in *sin;
  int len, lastlen, flags;
  char *buf, *ptr;
  iface_t *ift_head, **ift_next, *ift;
  int ret = SHOWNET_EOK;
  int sd;

  ift_head = NULL;
  ift_next = &ift_head;

  // create socket to get iface config
  sd = socket(PF_INET  ,SOCK_DGRAM , 0 );

  if(sd <0 ) {
    shownet_error("%s : Could not create socket %s", __FUNCTION__, strerror(errno) );
    ret = SHOWNET_ENET;
    goto e_return;
  }

  // first use ioctl to get a listing of interfaces
  lastlen = 0;
  len = INITIAL_IFACE_COUNT * sizeof(struct ifreq);

  for (;;) {
    buf = malloc(len);

    if(buf == NULL) {
      shownet_error_malloc();
      ret = SHOWNET_EMEM;
      goto e_free;
    }

    ifc.ifc_len = len;
    ifc.ifc_buf = buf;
    if ( ioctl(sd, SIOCGIFCONF, &ifc) < 0 ) {
      if ( errno != EINVAL || lastlen != 0 ) {
        shownet_error("%s : ioctl error %s" , __FUNCTION__, strerror(errno) );
        ret = SHOWNET_ENET;
        goto e_free;
      }
    } else {
      if(ifc.ifc_len == lastlen)
        break;
      lastlen = ifc.ifc_len;
    }
    len += IFACE_COUNT_INC;
    free(buf);
  }

  // loop through each iface
  for(ptr = buf; ptr < buf + ifc.ifc_len; ) {

    ifr = (struct ifreq* ) ptr;

    // work out length here
#ifdef HAVE_SOCKADDR_SA_LEN

    len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len );

#else
    switch (ifr->ifr_addr.sa_family) {
#ifdef  IPV6
      case AF_INET6:
        len = sizeof(struct sockaddr_in6);
        break;
#endif
      case AF_INET:
      default:
        len = sizeof(SA);
        break;
    }
#endif

    ptr += sizeof(ifr->ifr_name) + len;

    // look for AF_INET interfaces
    if(ifr->ifr_addr.sa_family == AF_INET) {
      ifrcopy = *ifr;
      if (ioctl(sd, SIOCGIFFLAGS, &ifrcopy) < 0) {
        shownet_error("%s : ioctl error %s" , __FUNCTION__, strerror(errno) );
        ret = SHOWNET_ENET;
        goto e_free_list;
      }

      flags = ifrcopy.ifr_flags;
      if( (flags & IFF_UP) == 0)
        continue; //skip down interfaces

      if( (flags & IFF_LOOPBACK) )
        continue; //skip lookback

      // interesting iface, better malloc for it ..
      ift = calloc(1, sizeof(iface_t) );

      if(ift == NULL) {
        shownet_error("%s : calloc error %s" , __FUNCTION__, strerror(errno) );
        ret = SHOWNET_EMEM;
        goto e_free_list;
      }

      if(ift_head == NULL) {
        ift_head = ift;
      } else {
        *ift_next = ift;
      }

      ift_next = &ift->next;

      sin = (struct sockaddr_in *) &ifr->ifr_addr;
      ift->ip_addr.sin_addr = sin->sin_addr;

      // fetch bcast address
#ifdef SIOCGIFBRDADDR
      if(flags & IFF_BROADCAST) {
        if (ioctl(sd, SIOCGIFBRDADDR, &ifrcopy) < 0) {
          shownet_error("%s : ioctl error %s" , __FUNCTION__, strerror(errno) );
          ret = SHOWNET_ENET;
          goto e_free_list;
        }

        sin = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
        ift->bcast_addr.sin_addr = sin->sin_addr;
      }
#endif
      // fetch hardware address
#ifdef SIOCGIFHWADDR
      if(flags & SIOCGIFHWADDR) {
        if (ioctl(sd, SIOCGIFHWADDR, &ifrcopy) < 0) {
          shownet_error("%s : ioctl error %s" , __FUNCTION__, strerror(errno) );
          ret = SHOWNET_ENET;
          goto e_free_list;
        }

        memcpy(&ift->hw_addr , ifrcopy.ifr_hwaddr.sa_data, 6);
      }
#endif

    /* ok, if that all failed we should prob try and use sysctl to work out the bcast
     * and hware addresses
     * i'll leave that for another day
     */

    } else {
      //
    }
  }
  *ift_head_r = ift_head;
  free(buf);
  return SHOWNET_EOK;

e_free_list:
  free_ifaces(ift_head);
e_free:
  free(buf);
  close(sd);
e_return:
  return ret;
}

#endif



/*
 * Check what interfaces exists, and what ip we will bind to
 *
 * @param n    shownet node
 * @param ip  requested ip (or NULL) if automatic
 */
int shownet_net_init(node n, const char *ip) {
  iface_t *ift_head, *ift;
  struct in_addr wanted_ip;

  int found = FALSE;
  int i, ret;

  if( (ret = get_ifaces(&ift_head)) )
    goto e_return;

  if(n->state.verbose) {
    printf("#### INTERFACES FOUND ####\n");
    for (ift = ift_head;ift != NULL; ift = ift->next ) {
      printf("IP: %s\n", inet_ntoa(ift->ip_addr.sin_addr) );
      printf("  bcast: %s\n" , inet_ntoa(ift->bcast_addr.sin_addr) );
      printf("  hwaddr: ");
        for(i = 0; i < 6; i++ ) {
          printf("%hhx:", ift->hw_addr[i] );
        }
      printf("\n");
    }
    printf("#########################\n");
  }

  if (ip == NULL) {
    if (ift_head != NULL) {
      // pick first address
      // copy ip address, bcast address and hardware address
      n->state.ip_addr = ift_head->ip_addr.sin_addr;
      n->state.bcast_addr = ift_head->bcast_addr.sin_addr;
    } else {
      shownet_error("No interfaces found!");
      ret = SHOWNET_ENET;
      goto e_cleanup;
    }
  } else {
    // search through list of interfaces for one with the correct address
    if (inet_aton(ip, &wanted_ip) == 0) {
      shownet_error("Cannot convert address %s", ip);
      ret = SHOWNET_ENET;
      goto e_cleanup;
    }

    for (ift = ift_head; ift != NULL; ift = ift->next) {
      if (ift->ip_addr.sin_addr.s_addr == wanted_ip.s_addr) {
        found = TRUE;
        n->state.ip_addr = ift->ip_addr.sin_addr;
        n->state.bcast_addr = ift->bcast_addr.sin_addr;

      }
    }
    if (!found) {
      shownet_error("Cannot find ip %s", ip);
      ret = SHOWNET_ENET;
      goto e_cleanup;
    }
  }

e_cleanup:
  free_ifaces(ift_head);
e_return :
  return ret;

}


/*
 * Start the network components of this node
 *
 * @param n  the shownet node
 */
int shownet_net_start(node n) {
  struct sockaddr_in servAddr;
  int bcast_flag = TRUE;
  int ret;

  /* create socket */
  n->sd = socket(PF_INET, SOCK_DGRAM, 0);

  if (n->sd <0) {
    shownet_error("Could not create socket %s", strerror(errno));
    ret = SHOWNET_ENET;
    goto e_socket1;
  }

  memset(&servAddr, 0x00, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(SHOWNET_PORT);
  servAddr.sin_addr.s_addr =  htonl(INADDR_ANY);

  if (n->state.verbose)
    printf("Binding to %s \n" , inet_ntoa(servAddr.sin_addr));

  /* bind sockets
   * we do one for the ip address and one for the broadcast address
   */
  if (bind(n->sd, (SA *) &servAddr, sizeof(servAddr)) == -1) {
    shownet_error("Failed to bind to socket %s", strerror(errno));
    ret = SHOWNET_ENET;
    goto e_bind1;
  }

  // allow bcasting
  if (setsockopt(n->sd, SOL_SOCKET, SO_BROADCAST, &bcast_flag, sizeof(int)) == -1) {
    shownet_error("Failed to bind to socket %s", strerror(errno));
    ret =  SHOWNET_ENET;
    goto e_setsockopt;
  }

  return SHOWNET_EOK;

e_setsockopt:
e_bind1:
  close(n->sd);

e_socket1:
  return ret;

}


/*
 * Recv a datagram
 */
ssize_t shownet_net_recv(node n, shownet_packet p, int delay) {
  ssize_t len;
  struct sockaddr_in cliAddr;
  socklen_t cliLen = sizeof(cliAddr);
  fd_set rset;
  struct timeval tv;
   int maxfdp1 =  n->sd + 1;

  FD_ZERO(&rset);
  FD_SET(n->sd, &rset);

  tv.tv_usec = 0;
  tv.tv_sec = delay;

  p->length = 0;

  switch (select(maxfdp1, &rset, NULL, NULL, &tv)) {
    case 0:
      // timeout
      return RECV_NO_DATA;
      break;
    case -1:
      if (errno != EINTR) {
        shownet_error("%s : select error", __FUNCTION__);
        return SHOWNET_ENET;
      }
      return SHOWNET_EOK;
      break;
    default:
      break;
  }


  // need a check here for the amount of data read
  // should prob allow an extra byte after data, and pass the size as sizeof(Data) +1
  // then check the size read and if equal to size(data)+1 we have an error
  if ((len = recvfrom(n->sd, &(p->data), sizeof(p->data), 0, (SA*) &cliAddr, &cliLen) )< 0) {
    shownet_error("%s : recvfrom error %s", __FUNCTION__, strerror(errno) );
    return SHOWNET_ENET;
  }

  if (cliAddr.sin_addr.s_addr == n->state.ip_addr.s_addr) {
    p->length = 0;
    return SHOWNET_EOK;
  }

  p->length = len;
  memcpy(&(p->from), &cliAddr.sin_addr, sizeof(struct in_addr));

  printf("p length %i\n", len);
  return SHOWNET_EOK;
}


/*
 * Send a datagram
 *
 */
int shownet_net_send(node n, shownet_packet p) {
  struct sockaddr_in addr;
  int ret;

  if (n->state.mode != SHOWNET_ON)
    return 0;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(SHOWNET_PORT);
  addr.sin_addr = p->to;
  p->from = n->state.ip_addr;

  if(n->state.verbose)
    printf("sending to %s\n", inet_ntoa(addr.sin_addr) );

  ret = sendto(n->sd, &p->data, p->length, 0, (SA*) &addr, sizeof(addr));
  if (ret == -1) {
    shownet_error("Sendto failed: %s", strerror(errno) );
    return SHOWNET_ENET;
  } else if (p->length != ret ) {
    shownet_error("failed to send full datagram");
    return SHOWNET_ENET;
  }

  return SHOWNET_EOK;
}


/*
 * Shutdown the networking components
 *
 * @param  n  the shownet node
 */
int shownet_net_close(node n) {
  close(n->sd);
  return SHOWNET_EOK;
}

