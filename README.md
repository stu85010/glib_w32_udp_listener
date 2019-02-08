# glib_w32_udp_listener
Listen UDP message by GLib at specific port

## Environment
* Visual Studio 2015 Update 3
* Windows 10
* [vcpkg](https://github.com/Microsoft/vcpkg)
* [GLib 2.52.3](https://github.com/Microsoft/vcpkg/blob/master/ports/glib/portfile.cmake#L18)

## Background
There's many discussion for the [g_poll could not work on Winsock issue](https://gitlab.gnome.org/GNOME/glib/issues/214), this issue made [Aravis](https://github.com/AravisProject/aravis) library could not be integrated well on Windows OS.
- [GNOME/GLib/Issues/g_poll does not work on win32 sockets](https://gitlab.gnome.org/GNOME/glib/issues/214)
- [GTK-Devel-List Mailing List/g_io_channel_win32_poll() Problem on Windows](https://www.mail-archive.com/gtk-devel-list@gnome.org/msg18571.html)
  -  [LRN Wed, 02 Aug 2017 09:45:50 -0700](https://www.mail-archive.com/gtk-devel-list@gnome.org/msg18585.html)
  - [LRN Fri, 28 Jul 2017 05:59:25 -0700](https://www.mail-archive.com/gtk-devel-list@gnome.org/msg18575.html)

And I am trying to figure out what can I do for Aravis porting for Windows. I'm planning to try below method and find out what's the most suitable modification for Aravis for Windows:
- [x] UDP receiving by GSource + GMainLoop
- [ ] UDP receiving by g_poll
- [ ] UDP receiving using native Winsock API
- [ ] Make a g_poll-like feature, replacing the g_poll code in Aravis
- [ ] Integrating "GSource + GMainLoop" in Aravis for Windows build
- [ ] UDP receiving by other polling method
- [ ] Complete the g_io_channel_win32_make_pollfd in GLib like LRN comment on the [thread](https://gitlab.gnome.org/GNOME/glib/issues/214#note_263930)
