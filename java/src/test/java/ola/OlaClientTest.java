package ola;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import ola.proto.Ola.DeviceConfigReply;
import ola.proto.Ola.DeviceInfoReply;
import ola.proto.Ola.DmxData;
import ola.proto.Ola.MergeMode;
import ola.proto.Ola.PatchAction;
import ola.proto.Ola.PluginDescriptionReply;
import ola.proto.Ola.PluginListReply;
import ola.proto.Ola.RDMResponse;
import ola.proto.Ola.RegisterAction;
import ola.proto.Ola.TimeCodeType;
import ola.proto.Ola.UID;
import ola.proto.Ola.UIDListReply;
import ola.proto.Ola.UniverseInfoReply;

import org.junit.Before;
import org.junit.Test;

/**
 * Test interaction with ola daemon.
 * Assumes that an ola daemon is running.
 */
public class OlaClientTest {

    private OlaClient client;

    @Before
    public void setup() throws Exception {
        if (client == null) {
            client = new OlaClient();
        }
    }

    @Test
    public void testGetPlugins() {
        PluginListReply reply = client.getPlugins();
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testGetPluginDescription() {
        PluginDescriptionReply reply = client.getPluginDescription(1);
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testGetDeviceInfo() {
        DeviceInfoReply reply = client.getDeviceInfo();
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testGetCandidatePorts() {
        DeviceInfoReply reply = client.getCandidatePorts(0);
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testConfigureDevice() {
        DeviceConfigReply reply = client.configureDevice(2, new short[] {200,200,200});
        // TODO verify result..
        System.out.println(reply);
    }

    @Test
    public void testGetUniverseInfo() {
        UniverseInfoReply reply = client.getUniverseInfo(0);
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testGetUIDs() {
        UIDListReply reply = client.getUIDs(0);
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testForceDiscovery() {
        UIDListReply reply = client.forceDiscovery(0, true);
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testGetDmx() {
        client.sendDmx(0, new short[] {45,12,244});
        DmxData reply = client.getDmx(0);
        short[] state = client.convertFromUnsigned(reply.getData());
        assertEquals(45, state[0]);
        assertEquals(12, state[1]);
        assertEquals(244, state[2]);

        System.out.println(reply);
    }

    @Test
    public void testPatchPort() {
        assertTrue(client.patchPort(1, 0, PatchAction.PATCH, 0));
    }

    @Test
    public void testSendDmx() {
        assertTrue(client.sendDmx(0, new short[] {10,23,244}));
    }

    @Test
    public void testSetPortPriority() {
        assertTrue(client.setPortPriority(1, 0, 0 ,0, true));
    }

    @Test
    public void testSetUniverseName() {
        client.setUniverseName(0, "outerspace");
        UniverseInfoReply reply = client.getUniverseInfo(0);
        assertEquals(reply.getUniverse(0).getName(), "outerspace");
    }

    @Test
    public void testSetMergeMode() {
        assertTrue(client.setMergeMode(0, MergeMode.HTP));
    }

    @Test
    public void testRegisterForDmx() {
        assertTrue(client.registerForDmx(0, RegisterAction.REGISTER));
    }

    @Test
    public void testSetSourceUID() {
        assertTrue(client.setSourceUID(1, 5));
    }

    @Test
    public void testSendTimeCode() {
        assertTrue(client.sendTimeCode(TimeCodeType.TIMECODE_DF, 10, 1, 1, 1));
    }

    @Test
    public void testSendRDMCommand() {
        UID id = UID.newBuilder()
                .setDeviceId(1)
                .setEstaId(9)
                .build();
        RDMResponse reply = client.sendRDMCommand(id, 0, 0, false, false, 0, new short[] {1,2,3});
        assertNotNull(reply);
        System.out.println(reply);
    }

    @Test
    public void testStreamDmx() {
        client.sendDmx(0, new short[] {9, 9, 9, 9});
        client.streamDmx(0, new short[] {14, 33, 55, 99});
        assertTrue(client.sendDmx(0, new short[] {9, 9, 9, 9}));
    }


    @Test
    public void testSendDmxRepetitive() throws Exception {

        OlaClient client = new OlaClient();

        for (int i = 0; i < 20; i++) {
            client.sendDmx(0, new short[] {135, 0, 0});
            Thread.currentThread().sleep(50);
            client.sendDmx(0, new short[] {135, 0, 135});
            Thread.currentThread().sleep(50);
        }

        for (short i = 0; i < 25; i+=3) {
            client.sendDmx(0, new short[] {i, 0, 0});
        }
        for (short i = 0; i < 25; i+=3) {
            client.sendDmx(0, new short[] {255, i, 0});
        }
        for (short i = 0; i < 25; i+=3) {
            client.sendDmx(0, new short[] {255, 255, i});
        }

        client.sendDmx(0, new short[] {0, 0, 0});
    }
}
