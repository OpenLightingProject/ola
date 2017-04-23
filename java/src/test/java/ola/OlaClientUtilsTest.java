package ola;

import static org.junit.Assert.assertArrayEquals;
import com.google.protobuf.ByteString;
import ola.OlaClient;

import org.junit.Before;
import org.junit.Test;

/**
 * Test standalone OlaClient code.
 * Does not require an ola daemon to be running.
 */
public class OlaClientUtilsTest {

    private OlaClient client;

    @Before
    public void setup() throws Exception {
        if (client == null) {
            client = new OlaClient();
        }
    }

    @Test
    public void testConvertToUnsigned() {
     	assertArrayEquals(client.convertToUnsigned(new short[]{0, 1, 2}).toByteArray(),
                          ByteString.copyFrom(new byte[]{0, 1, 2}).toByteArray());

    	assertArrayEquals(client.convertToUnsigned(new short[]{0xff, 128, 0}).toByteArray(),
                          ByteString.copyFrom(new byte[]{(byte)0xff, (byte)128, 0}).toByteArray());
    }

    @Test
    public void testConvertFromUnsigned() {
        assertArrayEquals(client.convertFromUnsigned(ByteString.copyFrom(new byte[]{0, 1, 2})),
                          new short[]{0, 1, 2});

        assertArrayEquals(
            client.convertFromUnsigned(ByteString.copyFrom(
                new byte[]{(byte)0xff, (byte)128, 0})),
            new short[]{0xff, 128, 0});
    }
}
