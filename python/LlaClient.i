
%module(directors="1") lla

%include "std_string.i"
%include "std_vector.i"
%include "carrays.i"

%{
#include <lla/LlaClient.h>
#include <lla/LlaClientObserver.h>
#include <lla/LlaPlugin.h>
#include <lla/LlaUniverse.h>
#include <lla/LlaDevice.h>
#include <lla/LlaPort.h>
#include <lla/plugin_id.h>
#include <lla/messages.h>
#include <lla/LlaUniverse.h>
%}

/*
 * turn on directors for the observer class so that the python methods can be
 * called from c++
 */
%feature("director") LlaClientObserver;

/*
 * maps vectors to lists
 */
namespace std {
   %template(UniverseVector) vector<LlaUniverse*>;
   %template(PluginVector) vector<LlaPlugin*>;
   %template(DeviceVector) vector<LlaDevice*>;
   %template(PortVector) vector<LlaPort*>;
};

/*
 * map uint8_t to ints
 */
%typemap(in) uint8_t {
    $1 = PyInt_AsLong($input);
}

%typemap(out) uint8_t {
    $result = PyInt_FromLong($1);
}

/*
 * Map uint8_t * to ArrayObjects
 */
%array_class(uint8_t, dmxBuffer);





enum lla_plugin_id {
    LLA_PLUGIN_ALL = 0,
    LLA_PLUGIN_DUMMY,
    LLA_PLUGIN_ARTNET,
    LLA_PLUGIN_SHOWNET,
    LLA_PLUGIN_ESPNET,
    LLA_PLUGIN_USBPRO,
    LLA_PLUGIN_OPENDMX,
    LLA_PLUGIN_SANDNET,
    LLA_PLUGIN_STAGEPROFI,
    LLA_PLUGIN_PATHPORT,
    LLA_PLUGIN_LAST
};


/*
 *
 */
class LlaClientObserver {
public:
    virtual ~LlaClientObserver();

    virtual int new_dmx(unsigned int uni, unsigned int length, uint8_t *data);
    virtual int universes(const std::vector<LlaUniverse*> unis);
    virtual int plugins(const std::vector<LlaPlugin*> plugins);
    virtual int devices(const std::vector<LlaDevice*> devices);
    virtual int ports(LlaDevice *dev);
    virtual int plugin_desc(LlaPlugin *plug);
    virtual int dev_config(unsigned int dev, uint8_t *req, unsigned int len);
};


/*
 *
 */
class LlaPlugin {
public:
  LlaPlugin(int id, const string &name);
  ~LlaPlugin();

  int get_id();
  std::string get_name();
  std::string get_desc();
  void set_desc(const std::string &desc);
};


/*
 *
 */
class LlaUniverse {
public:

  enum merge_mode { MERGE_HTP, MERGE_LTP };

  LlaUniverse(int id, merge_mode m, const std::string &name);
  ~LlaUniverse();

  int get_id();
  merge_mode get_merge_mode();
  std::string get_name();
};


/*
 *
 */
class LlaDevice {
public:
  LlaDevice(int id, int count, const std::string &name, int pid);
  ~LlaDevice();

  int get_id();
  int port_count();
  std::string get_name();
  int get_plugid();
  int add_port(class LlaPort *prt);
  int reset_ports();
  const std::vector<LlaPort*> get_ports();
};


/*
 *
 */
class LlaPort {
public:
  enum PortCapability { LLA_PORT_CAP_IN, LLA_PORT_CAP_OUT};

  LlaPort(int id, PortCapability cap, int uni, int active);
  ~LlaPort() {};

  int get_id();
  PortCapability get_capability();
  int get_uni();
  int is_active();
};

/*
 * LlaClient wrapper
 */
class LlaClient {
public:

  enum PatchAction {PATCH, UNPATCH};
  enum RegisterAction {REGISTER, UNREGISTER};

  LlaClient();
  ~LlaClient();
  int start();
  int stop();
  int fd() const;
  int fd_action(unsigned int delay);
  int set_observer(LlaClientObserver *o);

  // dmx methods
  int send_dmx(unsigned int universe, uint8_t *data, unsigned int length);
  int fetch_dmx(unsigned int uni);

  // rdm methods
  // int send_rdm(int universe, uint8_t *data, int length);

  int fetch_dev_info(lla_plugin_id filter);
  int fetch_port_info(LlaDevice *dev);
  int fetch_uni_info();
  int fetch_plugin_info();
  int fetch_plugin_desc(LlaPlugin *plug);

  int set_uni_name(unsigned int uni, const std::string &name);
  int set_uni_merge_mode(unsigned int uni, LlaUniverse::merge_mode mode);

  int register_uni(unsigned int uni, LlaClient::RegisterAction action);
  int patch(unsigned int dev, unsigned int port, LlaClient::PatchAction action, unsigned int uni);
  int dev_config(unsigned int dev, const class LlaDevConfMsg *msg);


};

