

#ifdef _MSC_VER
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>

#include <glib.h>
#include <gio/gio.h>


using namespace std;

#define LISTEN_PORT	(39217)

GMainLoop *loop = NULL;
GPollFD *poll_fds = NULL;
#define SOCKET_RECEV_BUFFER_SIZE	(1024)
GSocket *_socket = NULL;
GIOChannel *_channel = NULL;
int task_should_exit = 0;

#define G_SOURCE_METHOD	(0)
#define G_POLL_METHOD	(1)

static gboolean udp_received(GSocket *sock, GIOCondition condition, gpointer data)
{
	char buf[1024];
	gsize bytes_read = 0;
	GSocketAddress *address;
	GError *err = NULL;

	if (condition & G_IO_HUP) return FALSE;

	bytes_read = g_socket_receive_from(sock, &address, buf, sizeof(buf), NULL, &err);
	g_assert(err == NULL);

	g_print("UDP callback\n");
	if (bytes_read>0)
		g_print("UDP received %d bytes\n[%s]\n", bytes_read, buf);

	return TRUE;
}

#pragma region FIXED_WIN32_POLL
#ifdef _MSC_VER
#include <WinSock2.h>
#if 1
typedef struct _GIOWin32Channel GIOWin32Channel;
typedef struct _GIOWin32Watch GIOWin32Watch;

#define BUFFER_SIZE 4096

typedef enum {
	G_IO_WIN32_WINDOWS_MESSAGES,	/* Windows messages */

	G_IO_WIN32_FILE_DESC,		/* Unix-like file descriptors from
								* _open() or _pipe(), except for
								* console IO. Separate thread to read
								* or write.
								*/

	G_IO_WIN32_CONSOLE,		/* Console IO (usually stdin, stdout, stderr) */

	G_IO_WIN32_SOCKET		/* Sockets. No separate thread. */
} GIOWin32ChannelType;

struct _GIOWin32Channel {
	GIOChannel channel;
	gint fd;			/* Either a Unix-like file handle as provided
						* by the Microsoft C runtime, or a SOCKET
						* as provided by WinSock.
						*/
	GIOWin32ChannelType type;

	gboolean debug;

	/* Field used by G_IO_WIN32_WINDOWS_MESSAGES channels */
	HWND hwnd;			/* Handle of window, or NULL */

						/* Fields used by G_IO_WIN32_FILE_DESC channels. */
	CRITICAL_SECTION mutex;

	int direction;		/* 0 means we read from it,
						* 1 means we write to it.
						*/

	gboolean running;		/* Is reader or writer thread
							* running. FALSE if EOF has been
							* reached by the reader thread.
							*/

	gboolean needs_close;		/* If the channel has been closed while
								* the reader thread was still running.
								*/

	guint thread_id;		/* If non-NULL the channel has or has
							* had a reader or writer thread.
							*/
	HANDLE data_avail_event;

	gushort revents;

	/* Data is kept in a circular buffer. To be able to distinguish between
	* empty and full buffers, we cannot fill it completely, but have to
	* leave a one character gap.
	*
	* Data available is between indexes rdp and wrp-1 (modulo BUFFER_SIZE).
	*
	* Empty:    wrp == rdp
	* Full:     (wrp + 1) % BUFFER_SIZE == rdp
	* Partial:  otherwise
	*/
	guchar *buffer;		/* (Circular) buffer */
	gint wrp, rdp;		/* Buffer indices for writing and reading */
	HANDLE space_avail_event;

	/* Fields used by G_IO_WIN32_SOCKET channels */
	int event_mask;
	int last_events;
	HANDLE event;
	gboolean write_would_have_blocked;
	gboolean ever_writable;
};

struct _GIOWin32Watch {
	GSource       source;
	GPollFD       pollfd;
	GIOChannel   *channel;
	GIOCondition  condition;
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
static const char *
event_mask_to_string(int mask)
{
	char buf[100];
	int checked_bits = 0;
	char *bufp = buf;

	if (mask == 0)
		return "";

#define BIT(n) checked_bits |= FD_##n; if (mask & FD_##n) bufp += sprintf (bufp, "%s" #n, (bufp>buf ? "|" : ""))

	BIT(READ);
	BIT(WRITE);
	BIT(OOB);
	BIT(ACCEPT);
	BIT(CONNECT);
	BIT(CLOSE);
	BIT(QOS);
	BIT(GROUP_QOS);
	BIT(ROUTING_INTERFACE_CHANGE);
	BIT(ADDRESS_LIST_CHANGE);

#undef BIT

	if ((mask & ~checked_bits) != 0)
		bufp += sprintf(bufp, "|%#x", mask & ~checked_bits);

	return g_quark_to_string(g_quark_from_string(buf));
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

void fixed_g_io_channel_win32_make_pollfd(GIOChannel   *channel,
	GIOCondition  condition,
	GPollFD      *fd)
{
	GIOWin32Channel *win32_channel = (GIOWin32Channel *)channel;

	switch (win32_channel->type)
	{
	//case G_IO_WIN32_FILE_DESC:
	//	if (win32_channel->data_avail_event == NULL)
	//		create_events(win32_channel);

	//	fd->fd = (gintptr)win32_channel->data_avail_event;

	//	if (win32_channel->thread_id == 0)
	//	{
	//		/* Is it meaningful for a file descriptor to be polled for
	//		* both IN and OUT? For what kind of file descriptor would
	//		* that be? Doesn't seem to make sense, in practise the file
	//		* descriptors handled here are always read or write ends of
	//		* pipes surely, and thus unidirectional.
	//		*/
	//		if (condition & G_IO_IN)
	//			create_thread(win32_channel, condition, read_thread);
	//		else if (condition & G_IO_OUT)
	//			create_thread(win32_channel, condition, write_thread);
	//	}
	//	break;

	//case G_IO_WIN32_CONSOLE:
	//	fd->fd = _get_osfhandle(win32_channel->fd);
	//	break;

	case G_IO_WIN32_SOCKET:
#if 0
		fd->fd = (gintptr)WSACreateEvent();
#else
		if (win32_channel->event == WSA_INVALID_EVENT)
			win32_channel->event = WSACreateEvent();

		fd->fd = (gintptr)win32_channel->event;

		if (fd->fd == (gintptr)WSA_INVALID_EVENT) {
			gchar *emsg = g_win32_error_message(GetLastError());
			g_error("Error creating event: %s", emsg);
			g_free(emsg);
		} else {
			int event_mask = 0;

			if (condition & G_IO_IN)
				event_mask |= (FD_READ | FD_ACCEPT);
			if (condition & G_IO_OUT)
				event_mask |= (FD_WRITE | FD_CONNECT);
			event_mask |= FD_CLOSE;

			ResetEvent((WSAEVENT)fd->fd);

			if (win32_channel->debug)
				g_print("WSAEventSelect(%d,%p,{%s})",
					win32_channel->fd, (HANDLE)fd->fd,
					event_mask_to_string(event_mask));
			if (WSAEventSelect(win32_channel->fd, (HANDLE)fd->fd,
				event_mask) == SOCKET_ERROR)
				if (win32_channel->debug)
				{
					gchar *emsg = g_win32_error_message(WSAGetLastError());

					g_print(" failed: %s", emsg);
					g_free(emsg);
				}
			if (win32_channel->debug)
				g_print("\n");

			if ((event_mask & FD_WRITE) &&
				win32_channel->ever_writable &&
				!win32_channel->write_would_have_blocked)
			{
				if (win32_channel->debug)
					g_print("WSASetEvent(%p)\n", (WSAEVENT)fd->fd);
				WSASetEvent((WSAEVENT)fd->fd);
			}
		}
#endif	
		break;

	case G_IO_WIN32_WINDOWS_MESSAGES:
		fd->fd = G_WIN32_MSG_HANDLE;
		break;

	default:
		g_assert_not_reached();
		g_abort();
	}

	fd->events = condition;
}
#else
void fixed_g_io_channel_win32_make_pollfd(GIOChannel   *channel,
	GIOCondition  condition,
	GPollFD      *fd)
{
}
#endif
#endif // #ifdef _MSC_VER
#pragma endregion

void socket_hook_pollfd(GSocket *socket)
{
	gint fd = g_socket_get_fd(socket);
	
	_socket = socket;
	poll_fds = g_new(GPollFD, 1);

#ifdef _MSC_VER
	GIOChannel *channel = g_io_channel_win32_new_socket(fd);
	//g_io_channel_win32_make_pollfd(channel, G_IO_IN, poll_fds);
	fixed_g_io_channel_win32_make_pollfd(channel, G_IO_IN, poll_fds);
	//g_io_channel_ref(channel);
	_channel = channel;
	printf("poll fds: %X\n", poll_fds[0].fd);
#else
	GIOChannel *channel = g_io_channel_unix_new(fd);

	poll_fds[0].fd = fd;
	poll_fds[0].events = G_IO_IN;
	poll_fds[0].revents = 0;
#endif
}

void socket_hook_callback(GSocket *socket, GMainContext *context)
{
	gint fd = g_socket_get_fd(socket);
	GSource *source;

#ifdef _MSC_VER
	GIOChannel *channel = g_io_channel_win32_new_socket(fd);
#else
	GIOChannel *channel = g_io_channel_unix_new(fd);

	poll_fds[0].fd = fd;
	poll_fds[0].events = G_IO_IN;
	poll_fds[0].revents = 0;
#endif
	source = g_socket_create_source(socket, G_IO_IN, NULL);
	g_source_set_callback(source, (GSourceFunc)udp_received, NULL, NULL);
	g_source_attach(source, context);
}

#ifdef _MSC_VER
// https://stackoverflow.com/questions/18291284/handle-ctrlc-on-win32
BOOL WINAPI consoleHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		printf("Ctrl-C\n");
		g_main_loop_quit(loop);
		task_should_exit = 1;
		return TRUE;
		break;
	case CTRL_BREAK_EVENT:
		printf("Pause-Break\n");
		return TRUE;
		break;
	case CTRL_CLOSE_EVENT:
		printf("Close\n");
		task_should_exit = 1;
		break;
	default:
		printf("Other case: %d\n", dwCtrlType);
	}

	return FALSE;
}
#endif

void print_buffer(char *buf, int length)
{
	for (int i = 0; i < length; i++) {
		printf("%c", buf[i]);
	}
	fflush(stdout);
}

gboolean timeout_callback(gpointer data)
{
	static int isConnected = 0;
	if (poll_fds != NULL) {
		int ret = 0;
		if (!isConnected) {
			//std::cout << "use poll!" << std::endl;
			isConnected = 1;
		}
		ret = g_poll(poll_fds, 1, 1000);
		//ret = g_io_channel_win32_poll(poll_fds, 1, 1000);
		//ret = g_socket_condition_timed_wait(_socket, G_IO_IN, 1000, NULL, NULL);

		if (ret == 0) {
			//std::cout << "poll zero" << std::endl;

		} else {
			char buffer[SOCKET_RECEV_BUFFER_SIZE];
			int recv_len = 0;
			
			GSocketAddress *address;
			GError *err = NULL;
			WSANETWORKEVENTS events;
			int socket_fd = g_socket_get_fd(_socket);
			int fd = poll_fds[0].fd;
			if (WSAEnumNetworkEvents(socket_fd,
				(HANDLE) fd,
				&events) == 0) {
				std::cout << "WSAEnumNetworkEvents event: " << events.lNetworkEvents << endl;

			} else {
				std::cout << "enum event failed" << endl;
			}

			//std::cout << "poll not zero, ret: " << ret << std::endl;
			do {
				//printf("events: %X, revents: %X\n", poll_fds->events, poll_fds->revents);
				g_socket_set_blocking(_socket, FALSE);
				//recv_len = g_socket_receive(_socket, buffer, SOCKET_RECEV_BUFFER_SIZE, NULL, NULL);
				recv_len = g_socket_receive_from(_socket, &address, buffer, SOCKET_RECEV_BUFFER_SIZE, NULL, NULL);
				//recv_len = g_socket_receive_from(_socket, &address, buffer, 10, NULL, NULL);
				//_channel->funcs->io_read(_channel, buffer, 10, &recv_len, NULL);

				GInetAddress *iaddr = g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(address));
				int src_port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(address));
				char *addr_str = g_inet_address_to_string(iaddr);
				std::cout << "send by: " << addr_str << ", from port:" << src_port << endl;
				free(addr_str);

				g_socket_set_blocking(_socket, TRUE);

				if (err) {
					g_error(err->message);
				}

				if (recv_len > 0) {
					std::cout << "received length: " << recv_len << ", you got [";
					print_buffer(buffer, recv_len);
					std::cout << "]." << std::endl;
					recv_len -= recv_len;
				}
			} while (recv_len > 0);


		}

	}
	return TRUE;
}

void main_loop(void)
{
	while (!task_should_exit) {
		timeout_callback(NULL);
		Sleep(100);
	}
}

int main(int argc, char *argv[])
{
	std::cout << "This is UDP socket listener!!" << std::endl;
	int result = _putenv("G_IO_WIN32_DEBUG=1");
	//_putenv("G_MAIN_POLL_DEBUG=1");
	
	//cout << "putenv result: " << result << endl;

	//GInetAddress *address = g_inet_address_new_from_string("127.0.0.1");
	//GInetAddress *address = g_inet_address_new_from_string("192.168.0.9"); // ip from LAN
	GInetAddress *address = g_inet_address_new_any(G_SOCKET_FAMILY_IPV4);
	GSocketAddress *socket_address = g_inet_socket_address_new(address, LISTEN_PORT);
	GError *error = NULL;

	GSocket *socket = g_socket_new(G_SOCKET_FAMILY_IPV4,
									G_SOCKET_TYPE_DATAGRAM,
									G_SOCKET_PROTOCOL_UDP,
									&error);
	g_object_ref(socket);
	g_object_ref(socket_address);
	if (error) {
		g_error(error->message);
	}
	else {
		g_message("Socket creation ok");
	}

	g_socket_set_broadcast(socket, TRUE);
	if (g_socket_bind(socket, socket_address, FALSE, &error) == FALSE) {
		if (error) {
			g_error(error->message);
		}
		else {
			g_message("Socket bind ok");
		}
		return -1;
	}
	g_object_unref(socket_address);
	g_object_unref(address);
	
	GMainContext *context = NULL;
#if 1
	context = g_main_context_new();
#endif
	if (context) g_main_context_ref(context);

#if (G_SOURCE_METHOD)
	socket_hook_callback(socket, context);
#elif (G_POLL_METHOD)
	// G_POLL_METHOD
	socket_hook_pollfd(socket);
#endif

	g_socket_set_blocking(socket, FALSE);
	g_socket_set_broadcast(socket, TRUE);

#ifdef _MSC_VER
	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
		printf("\nERROR: Could not set control handler");
		return 1;
	}
#endif

	GSource *source = g_timeout_source_new(1000);
	loop = g_main_loop_new(context, FALSE);
	g_main_loop_ref(loop);

	//set the callback for this source
	g_source_set_callback(source, timeout_callback, loop, NULL);

	g_source_attach(source, context);

#if (G_SOURCE_METHOD)
	g_main_loop_run(loop);
#elif (G_POLL_METHOD)
	main_loop();
#endif

	g_main_loop_unref(loop);
	if (context) g_main_context_unref(context);
	g_source_unref(source);
	g_object_unref(socket);
	//WSACloseEvent((HANDLE)poll_fds[0].fd);
	g_io_channel_unref(_channel);

	std::cout << "end of UDP socket listener!" << std::endl;
	system("pause");
	return 0;
}
