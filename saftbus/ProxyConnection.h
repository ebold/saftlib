#ifndef SAFTBUS_PROXY_CONNECTION_H_
#define SAFTBUS_PROXY_CONNECTION_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <giomm.h>
//#include <giomm/asyncresult.h>

#include <thread>
#include <map>

#include "Interface.h"
#include "saftbus.h"


namespace saftbus
{

	class Proxy;

	// This class mimics the Gio::DBus::Connection object, but is limited to the functionality that is needed
	// on the Proxy side. The connection management on the Service side is done in the saftbus::Connection class.
	// This is different from the DBus interface where there is only one single Gio::DBus::Connection class. 
	// DBus is a many-to-many communication protocol, while saftbus is only a one-to-many protocoll. 
	class ProxyConnection : public Glib::Object//Base
	{
	public:
		ProxyConnection(const Glib::ustring &base_name = "/tmp/saftbus_");

		using SlotSignal = sigc::slot<void, const Glib::RefPtr<ProxyConnection>&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&>;

		// is used by Proxies to fetch properties
		Glib::VariantContainerBase& call_sync(const Glib::ustring& object_path, 
											const Glib::ustring& interface_name, 
											const Glib::ustring& method_name, 
											const Glib::VariantContainerBase& parameters, 
											const Glib::ustring& bus_name=Glib::ustring(), 
											int timeout_msec=-1);


		using SlotAsyncReady = sigc::slot<void, Glib::RefPtr<Gio::AsyncResult>&>;
		void call(const Glib::ustring& object_path,
				  const Glib::ustring& interface_name,
				  const Glib::ustring& method_name,
				  const Glib::VariantContainerBase& parameters,
				  const SlotAsyncReady& slot,
				  const Glib::RefPtr< Gio::Cancellable >& cancellable,
				  const Glib::RefPtr< Gio::UnixFDList >& fd_list,
				  const Glib::ustring& bus_name);
		Glib::VariantContainerBase& call_finish(const Glib::RefPtr< Gio::AsyncResult >&res);

	// internal stuff (not part the DBus fake api)
	public:
		void send_signal_flight_time(double signal_flight_time);
		void send_proxy_signal_fd(int pipe_fd, 
                                  Glib::ustring object_path,
                                  Glib::ustring interface_name,
                                  int global_id);
		void remove_proxy_signal_fd(Glib::ustring object_path,
                                    Glib::ustring interface_name,
                                    int global_id) ;

		Glib::ustring get_saftbus_id() { return _saftbus_id; }
		int get_connection_id();

		// returned fd should only be used with a lock or in single threaded environments
		int get_fd() const {return _create_socket; }
	private:

		bool on_async_call_reply(Glib::IOCondition condition, const SlotAsyncReady& slot, const Glib::RefPtr< Gio::UnixFDList >& fd_list);

		// this is the information that is needed to keep connected to a socket
		int _create_socket;
		struct sockaddr_un _address;
		std::string _filename;


		Glib::Variant<std::vector<Glib::VariantBase> > _call_sync_result;
		std::vector<char> _call_sync_result_buffer;

		Glib::VariantContainerBase _result;


		Glib::ustring _saftbus_id; 

		int _connection_id;

		std::mutex _socket_mutex;
	};

}


#endif