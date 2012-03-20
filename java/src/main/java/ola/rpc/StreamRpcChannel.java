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
package ola.rpc;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.logging.Level;
import java.util.logging.Logger;

import ola.proto.Ola.STREAMING_NO_RESPONSE;
import ola.rpc.Rpc.RpcMessage;

import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcChannel;
import com.google.protobuf.RpcController;

/**
 * Basic RPC Channel implementation.  All calls are done
 * synchronously.
 *
 * The RPC Channel is hard coded to localhost 9010 where the
 * olad daemon is running.
 */
public class StreamRpcChannel implements RpcChannel {

    private static Logger logger = Logger.getLogger(StreamRpcChannel.class.getName());

    public static final int PORT = 9010;

    public static final String HOST = "localhost";

    private static final int PROTOCOL_VERSION = 1;

    private static final int VERSION_MASK = 0xf0000000;

    private static final int SIZE_MASK = 0x0fffffff;

    private Socket socket = null;

    private BufferedOutputStream bos;

    private BufferedInputStream bis;

    private int sequence = 0;


    /**
     * Create new Rpc Channel Connection to olad.
     * @throws Exception
     */
    public StreamRpcChannel() throws Exception {
        connect();
    }


    /**
     * Open connection to olad daemon.
     *
     * @throws Exception
     */
    public void connect() throws Exception {

        if (socket != null && socket.isConnected()) {
            logger.warning("Socket already connected.");
            return;
        }

        try {
            socket = new Socket(HOST, PORT);
            bos = new BufferedOutputStream(socket.getOutputStream());
            bis = new BufferedInputStream(socket.getInputStream());
        } catch (Exception e) {
            logger.severe("Error connecting. Make sure the olad daemon is running on port 9010");
            throw e;
        }
    }


    /**
     * Close Rpc Channel.
     */
    public void close() {

        if (socket != null && socket.isConnected()) {
            try {
                socket.close();
            } catch (Exception e) {
                logger.warning("Error closing socket. " + e.getMessage());
            }
        }
    }


    /* (non-Javadoc)
     * @see com.google.protobuf.RpcChannel#callMethod(com.google.protobuf.Descriptors.MethodDescriptor, com.google.protobuf.RpcController, com.google.protobuf.Message, com.google.protobuf.Message, com.google.protobuf.RpcCallback)
     */
    public void callMethod(MethodDescriptor method, RpcController controller,
            Message requestMessage, Message responseMessage, RpcCallback<Message> done) {

        int messageId = sequence++;

        RpcMessage message = RpcMessage.newBuilder()
                .setType(Rpc.Type.REQUEST)
                .setId(messageId)
                .setName(method.getName())
                .setBuffer(requestMessage.toByteString())
                .build();

        try {

            sendMessage(message);
            if (responseMessage.getDescriptorForType().getName().equals("STREAMING_NO_RESPONSE")) {
                // don't wait for response on streaming messages..
                return;
            }

            RpcMessage response = readMessage();

            if (response.getType().equals(Rpc.Type.RESPONSE)) {
                if (response.getId() != messageId) {
                    controller.setFailed("Received message with id " + response.getId() + " , but was expecting " + messageId);
                } else {
                    responseMessage = DynamicMessage.parseFrom(responseMessage.getDescriptorForType(), response.getBuffer());
                    if (done != null) {
                        done.run(responseMessage);
                    }
                }
            } else {
                controller.setFailed("No valid response received !");
            }


        } catch (Exception e) {

            logger.severe("Error sending rpc message: " + e.getMessage());
            controller.setFailed(e.getMessage());
            done.run(null);
        }


    }


    /**
     * Send rpc message to olad.
     *
     * @param msg RpcMessage
     * @throws Exception
     */
    private void sendMessage(RpcMessage msg) throws Exception {

        byte[] data = msg.toByteArray();

        int headerContent = (PROTOCOL_VERSION << 28) & VERSION_MASK;
        headerContent |= data.length & SIZE_MASK;
        byte[] header = ByteBuffer.allocate(4).order(ByteOrder.nativeOrder()).putInt(headerContent).array();

        if (logger.isLoggable(Level.FINEST)) {
            logger.info("Sending header " + header.length + " bytes");
            for (byte b : header) {
                System.out.format("0x%x ", b);
             }
            logger.info("Sending data " + data.length + " bytes");
            for (byte b : data) {
                 System.out.format("0x%x ", b);
              }
        }

        bos.write(header);
        bos.write(data);
        bos.flush();
    }


    /**
     * @return RpcMessage read back from olad.
     *
     * @throws Exception
     */
    private RpcMessage readMessage() throws Exception {

        byte[] header = new byte[4];
        bis.read(header);

        int headerValue = ByteBuffer.wrap(header).order(ByteOrder.nativeOrder()).getInt();
        int size = headerValue  & SIZE_MASK;

        byte[] data = new byte[size];
        bis.read(data);

        if (logger.isLoggable(Level.FINEST)) {
            logger.info("Received header ");
            for (byte b : header) {
                    System.out.format("0x%x ", b);
            }
            logger.info("Received data ");
            for (byte b : data) {
                    System.out.format("0x%x ", b);
            }
        }

        return RpcMessage.parseFrom(data);
    }
}
