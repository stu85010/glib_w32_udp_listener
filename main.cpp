

#include <windows.h>

#include <string>
#include <vector>
#include <iostream>

#include <glib.h>
#include <gio/gio.h>


using namespace std;

#define LISTEN_PORT	(39217)

GMainLoop *loop = NULL;
GPollFD *poll_fds = NULL;

static gboolean udp_received(GSocket *sock, GIOCondition condition, gpointer data)
{
	char buf[1024];
	gsize bytes_read;
	GSocketAddress *address;
	GError *err = NULL;

	if (condition & G_IO_HUP) return FALSE;

	bytes_read = g_socket_receive_from(sock, &address, buf, sizeof(buf), NULL, &err);
	g_assert(err == NULL);

	g_print("UDP received %d bytes\n[%s]\n", bytes_read, buf);
	
	return TRUE;
}

void socket_hook_pollfd(GSocket *socket)
{
	gint fd = g_socket_get_fd(socket);
	poll_fds = g_new(GPollFD, 1);

#ifdef _MSC_VER
	GIOChannel *channel = g_io_channel_win32_new_socket(fd);
	g_io_channel_win32_make_pollfd(channel, G_IO_IN, poll_fds);
#else
	GIOChannel *channel = g_io_channel_unix_new(fd);

	poll_fds[0].fd = fd;
	poll_fds[0].events = G_IO_IN;
	poll_fds[0].revents = 0;
#endif
}

void socket_hook_callback(GSocket *socket)
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
	g_source_attach(source, NULL);
}

// https://stackoverflow.com/questions/18291284/handle-ctrlc-on-win32
BOOL WINAPI consoleHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		printf("Ctrl-C\n");
		g_main_loop_quit(loop);
		return TRUE;
		break;
	case CTRL_BREAK_EVENT:
		printf("Pause-Break\n");
		return TRUE;
		break;
	case CTRL_CLOSE_EVENT:
		printf("Close\n");
		break;
	default:
		printf("Other case: %d\n", dwCtrlType);
	}

	return FALSE;
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
		//ret = g_poll(poll_fds, 1, 90);
		ret = g_io_channel_win32_poll(poll_fds, 1, 90);

		if (ret == 0) {
			//std::cout << "poll zero" << std::endl;

		}
		else {
			std::cout << "poll not zero, ret: " << ret << std::endl;

		}

	}
	return TRUE;
}

int main(std::vector<std::string> args)
{
	std::cout << "This is UDP socket listener!!" << std::endl;

	//GInetAddress *address = g_inet_address_new_from_string("127.0.0.1");
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
	
	//socket_hook_pollfd(socket);
	socket_hook_callback(socket);

	g_socket_set_blocking(socket, FALSE);
	g_socket_set_broadcast(socket, TRUE);

	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
		printf("\nERROR: Could not set control handler");
		return 1;
	}

	GSource *source = g_timeout_source_new(100);
	//GMainContext *context = g_main_context_default();
	//GMainContext *context = g_main_context_new();
	//loop = g_main_loop_new(context, FALSE);
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_ref(loop);

	//set the callback for this source
	g_source_set_callback(source, timeout_callback, loop, NULL);

	//g_source_attach(source, context);
	g_source_attach(source, NULL);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	//g_object_unref(context);
	g_source_unref(source);

	std::cout << "end of UDP socket listener!" << std::endl;
	
	return 0;
}
