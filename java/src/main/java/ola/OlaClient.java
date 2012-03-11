/***********************************************************************
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *************************************************************************/

package ola;

import java.util.logging.Logger;

import ola.proto.Ola.DeviceConfigReply;
import ola.proto.Ola.DeviceConfigRequest;
import ola.proto.Ola.DeviceInfoReply;
import ola.proto.Ola.DeviceInfoRequest;
import ola.proto.Ola.DiscoveryRequest;
import ola.proto.Ola.DmxData;
import ola.proto.Ola.MergeMode;
import ola.proto.Ola.MergeModeRequest;
import ola.proto.Ola.OlaServerService;
import ola.proto.Ola.OptionalUniverseRequest;
import ola.proto.Ola.PatchAction;
import ola.proto.Ola.PatchPortRequest;
import ola.proto.Ola.PluginDescriptionReply;
import ola.proto.Ola.PluginDescriptionRequest;
import ola.proto.Ola.PluginListReply;
import ola.proto.Ola.PluginListRequest;
import ola.proto.Ola.PortPriorityRequest;
import ola.proto.Ola.RDMRequest;
import ola.proto.Ola.RDMResponse;
import ola.proto.Ola.RegisterAction;
import ola.proto.Ola.RegisterDmxRequest;
import ola.proto.Ola.TimeCode;
import ola.proto.Ola.TimeCodeType;
import ola.proto.Ola.UID;
import ola.proto.Ola.UIDListReply;
import ola.proto.Ola.UniverseInfoReply;
import ola.proto.Ola.UniverseNameRequest;
import ola.proto.Ola.UniverseRequest;
import ola.rpc.SimpleRpcController;
import ola.rpc.StreamRpcChannel;

import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcChannel;
import com.google.protobuf.RpcController;

public class OlaClient {

    private static Logger logger = Logger.getLogger(OlaClient.class.getName());

    private OlaServerService serverService;

    private RpcController controller;

    private RpcChannel channel;


    public OlaClient() throws Exception {

        channel = new StreamRpcChannel();
        controller = new SimpleRpcController();
        serverService = OlaServerService.Stub.newStub(channel);
    }


    /**
     * Generic method for making Rpc Calls.
     *
     * @param method Name of te Rpc Method to call
     * @param inputMessage Input RpcMessage
     * @return Message result message or null if the call failed.
     */
    private Message callRpcMethod(String method, Message inputMessage) {

        final Message[] outputMessage = new Message[1];
        controller.reset();

        RpcCallback<Message> cb = new RpcCallback<Message>() {
            public void run(Message arg0) {
                outputMessage[0] = arg0;
            }
        };

        serverService.callMethod(serverService.getDescriptorForType().findMethodByName(method), controller, inputMessage, cb);

        if (controller.failed()) {
            logger.warning("RPC Call failed: " + controller.errorText());
            return null;
        }

        return outputMessage[0];
    }



    /**
     * Get a list of plugins from olad.
     *
     * @return The list of plugings.
     */
    public PluginListReply getPlugins() {
        return (PluginListReply) callRpcMethod("GetPlugins", PluginListRequest.newBuilder().build());
    }


    /**
     * Get a plugin description from olad.
     *
     * @param pluginId number of the plugin for which to receive the description
     * @return The list of plugings.
     */
    public PluginDescriptionReply getPluginDescription(int pluginId) {

        PluginDescriptionRequest request = PluginDescriptionRequest.newBuilder()
                .setPluginId(pluginId)
                .build();

        return (PluginDescriptionReply) callRpcMethod("GetPluginDescription", request);
    }


    /**
     * Get device info from olad.
     *
     * @return The Device Info.
     */
    public DeviceInfoReply getDeviceInfo() {
        return (DeviceInfoReply) callRpcMethod("GetDeviceInfo", DeviceInfoRequest.newBuilder().build());
    }

    /**
     * Get candidate ports for universe.
     *
     * @param universe the id of the universe.
     * @return device info
     */
    public DeviceInfoReply getCandidatePorts(int universe) {
        OptionalUniverseRequest request = OptionalUniverseRequest.newBuilder().setUniverse(universe).build();
        return (DeviceInfoReply) callRpcMethod("GetCandidatePorts", request);
    }


    /**
     * Configure device.
     *
     * @param device the id of the device to configure.
     * @param data device configuration data.
     * @return
     */
    public DeviceConfigReply configureDevice(int device, short[] data) {
        DeviceConfigRequest request = DeviceConfigRequest.newBuilder()
                .setDeviceAlias(device)
                .setData(convertToUnsigned(data))
                .build();
        return (DeviceConfigReply) callRpcMethod("ConfigureDevice", request);
    }


    /**
     * Get universe information.
     *
     * @param universe the id of the universe
     * @return UniverseInfo
     */
    public UniverseInfoReply getUniverseInfo(int universe) {
        OptionalUniverseRequest request = OptionalUniverseRequest.newBuilder().setUniverse(universe).build();
        return (UniverseInfoReply) callRpcMethod("GetUniverseInfo", request);
    }


    /**
     * Get UID's.
     *
     * @param universe the id of the universe
     * @return UIDListReply
     */
    public UIDListReply getUIDs(int universe) {
        UniverseRequest request = UniverseRequest.newBuilder().setUniverse(universe).build();
        return (UIDListReply) callRpcMethod("GetUIDs", request);
    }


    /**
     * Force discovery of a universe.
     *
     * @param universe the id of the universe
     * @param full
     * @return UID List
     */
    public UIDListReply forceDiscovery(int universe, boolean full) {
        DiscoveryRequest request = DiscoveryRequest.newBuilder()
                .setUniverse(universe)
                .setFull(full)
                .build();
        return (UIDListReply) callRpcMethod("ForceDiscovery", request);
    }


    /**
     * Retrieve dmx data from universe.
     * @param universe the id of the universe
     * @return
     */
    public DmxData getDmx(int universe) {
        return (DmxData) callRpcMethod("GetDmx", UniverseRequest.newBuilder().setUniverse(universe).build());
    }



    /**
     * Patch a port.
     *
     * @param device number
     * @param port number
     * @param action PachAction.PATCH or PatchAction.UNPATCH
     * @param universe number
     * @return true when succeeded.
     */
    public boolean patchPort(int device, int port, PatchAction action, int universe) {

        PatchPortRequest patchRequest = PatchPortRequest.newBuilder()
                .setPortId(port)
                .setAction(action)
                .setDeviceAlias(device)
                .setUniverse(universe)
                .setIsOutput(true)
                .build();

        return callRpcMethod("PatchPort", patchRequest) != null;
    }


    /**
     * Send dmx data to olad.
     *
     * @param universe number
     * @param values array of dmx data values
     * @return true when succeeded.
     */
    public boolean sendDmx(int universe, short[] values) {

        DmxData dmxData = DmxData.newBuilder()
                .setUniverse(universe)
                .setData(convertToUnsigned(values))
                .build();

        return callRpcMethod("UpdateDmxData", dmxData) != null;
    }


    /**
     * Set port priority.
     *
     * @return true if request succeeded.
     */
    public boolean setPortPriority(int device, int port, int priority, int mode, boolean output) {
        PortPriorityRequest request = PortPriorityRequest.newBuilder()
            .setDeviceAlias(device)
            .setPortId(port)
            .setPriority(priority)
            .setPriorityMode(mode)
            .setIsOutput(output)
            .build();

        return callRpcMethod("SetPortPriority", request) != null;
    }


    /**
     * Set universe name.
     *
     * @param universe id of universe for which to set the name.
     * @param name The name to set.
     * @return true if the call succeeded.
     */
    public boolean setUniverseName(int universe, String name) {
        UniverseNameRequest request = UniverseNameRequest.newBuilder()
                .setUniverse(universe)
                .setName(name)
                .build();
        return callRpcMethod("SetUniverseName", request) != null;
    }


    /**
     * Define merge mode for a universe.
     *
     * @param universe The id of the universe
     * @param mode, merge mode to use
     * @return true if call succeeded.
     */
    public boolean setMergeMode(int universe, MergeMode mode) {
        MergeModeRequest request = MergeModeRequest.newBuilder()
                .setUniverse(universe)
                .setMergeMode(mode)
                .build();
        return callRpcMethod("SetMergeMode", request) != null;
    }


    /**
     * Register for dmx
     * @param universe
     * @param action RegisterAction
     * @return true if call succeeded.
     */
    public boolean registerForDmx(int universe, RegisterAction action) {
        RegisterDmxRequest request = RegisterDmxRequest.newBuilder()
                .setUniverse(universe)
                .setAction(action)
                .build();
        return callRpcMethod("RegisterForDmx", request) != null;
    }


    /**
     * Set source UID for device.
     * @param device The id of the device
     * @param estaId the UID to set.
     * @return true if call succeeded.
     */
    public boolean setSourceUID(int device, int estaId) {
        UID request = UID.newBuilder()
                .setDeviceId(device)
                .setEstaId(estaId)
                .build();
        return callRpcMethod("SetSourceUID", request) != null;
    }


    /**
     * Send TimeCode.
     *
     * @param type TimeCodeType
     * @param frames number of frames
     * @param hours
     * @param minutes
     * @param seconds
     * @return true if call succeeded.
     */
    public boolean sendTimeCode(TimeCodeType type, int frames, int hours, int minutes, int seconds) {
        TimeCode request = TimeCode.newBuilder()
                .setFrames(frames)
                .setHours(hours)
                .setMinutes(minutes)
                .setSeconds(seconds)
                .setType(type)
                .build();
        return callRpcMethod("SendTimeCode", request) != null;
    }


    /**
     * Send RDM Command.
     *
     * @param uid
     * @param subDevice
     * @param paramId
     * @param isSet
     * @param includeRawResponse
     * @param universe
     * @param data
     * @return RDMResponse
     */
    public RDMResponse sendRDMCommand(UID uid, int subDevice, int paramId, boolean isSet, boolean includeRawResponse, int universe, short[] data) {
        RDMRequest request = RDMRequest.newBuilder()
                .setUid(uid)
                .setSubDevice(subDevice)
                .setParamId(paramId)
                .setIsSet(isSet)
                .setIncludeRawResponse(includeRawResponse)
                .setUniverse(universe)
                .setData(convertToUnsigned(data))
                .build();
        return (RDMResponse) callRpcMethod("RDMCommand", request);
    }


    /**
     * Send dmx data, but don't wait for response.
     *
     * @param universe the id of the universe
     * @param values dmx data
     */
    public void streamDmx(int universe, short[] values) {

        DmxData dmxData = DmxData.newBuilder()
                .setUniverse(universe)
                .setData(convertToUnsigned(values))
                .build();

        callRpcMethod("StreamDmxData", dmxData);
    }


    /**
     * Convert short array to bytestring
     */
    public ByteString convertToUnsigned(short[] values) {
        byte[] unsigned = new byte[values.length];
        for (int i = 0; i < values.length; i++) {
            unsigned[i] = (byte) values[i];
        }
        return ByteString.copyFrom(unsigned);
    }


    /**
     * Convert bytestring to short array.
     */
    public short[] convertFromUnsigned(ByteString data) {
        byte[] values = data.toByteArray();
        short[] signed = new short[values.length];
        for (int i = 0; i < values.length; i++) {
            signed[i] = (short) ((short) values[i] & 0xFF);
        }
        return signed;
    }
}
