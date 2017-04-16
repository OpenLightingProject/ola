package ola.rpc;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import ola.rpc.Rpc.RpcMessage;
import ola.rpc.Rpc.Type;

import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcController;
import com.google.protobuf.Service;

public class StreamRpcChannelReadThread extends Thread {
    private static Logger logger = Logger
	    .getLogger(StreamRpcChannelReadThread.class.getName());

    private static final int SIZE_MASK = 0x0fffffff;

    protected class ExpectedResponse {
	private final RpcController controller;
	private final RpcCallback<Message> done;
	private final Message responseMessage;

	public ExpectedResponse(RpcController controller,
		RpcCallback<Message> done, Message responseMessage) {
	    this.controller = controller;
	    this.done = done;
	    this.responseMessage = responseMessage;
	}
    }

    private final Socket socket;
    private final BufferedInputStream bis;
    private Map<Integer, ExpectedResponse> expectedResponses = new HashMap<Integer, ExpectedResponse>();
    private final Service service;
    private final RpcController controller;

    public StreamRpcChannelReadThread(Socket socket, Service service)
	    throws IOException {
	this.socket = socket;
	this.bis = new BufferedInputStream(socket.getInputStream());
	this.service = service;
	this.controller = new SimpleRpcController();
    }

    public ExpectedResponse scheduleExpectedResponse(int id,
	    RpcController controller, RpcCallback<Message> done,
	    Message responseMessage) {
	ExpectedResponse expectedResponse = new ExpectedResponse(controller,
		done, responseMessage);
	this.expectedResponses.put(id, expectedResponse);
	return expectedResponse;
    }

    private void handleResponse(final RpcMessage response)
	    throws InvalidProtocolBufferException {
	if (!expectedResponses.containsKey(response.getId())) {
	    logger.warning("Received unexpected response message with id "
		    + response.getId());
	    return;
	}

	final ExpectedResponse expectedResponse = expectedResponses
		.remove(response.getId());

	RpcCallback<Message> done = expectedResponse.done;
	RpcController controller = expectedResponse.controller;

	synchronized (expectedResponse) {
	    if (response.getType() == Type.RESPONSE) {
		Message responseMessage = DynamicMessage
			.parseFrom(expectedResponse.responseMessage
				.getDescriptorForType(), response.getBuffer());
		if (done != null) {
		    done.run(responseMessage);
		}
	    } else {
		controller.setFailed(response.getBuffer().toStringUtf8());
		done.run(null);
	    }
	    expectedResponse.notify();
	}
    }

    private Message handleRequest(RpcMessage request)
	    throws InvalidProtocolBufferException {
	MethodDescriptor methodDescriptor = this.service.getDescriptorForType()
		.findMethodByName(request.getName());

	Message req = this.service.getRequestPrototype(methodDescriptor)
		.newBuilderForType().clear().mergeFrom(request.getBuffer())
		.build();

	final Message[] outputMessage = new Message[1];
	controller.reset();

	RpcCallback<Message> cb = new RpcCallback<Message>() {
	    public void run(Message arg0) {
		outputMessage[0] = arg0;
	    }
	};

	this.service.callMethod(methodDescriptor, controller, req, cb);

	if (controller.failed()) {
	    logger.warning("RPC Call failed: " + controller.errorText());
	    return null;
	}

	return outputMessage[0];
    }

    @Override
    public void run() {
	while (!this.socket.isClosed()) {
	    try {
		RpcMessage message = readMessage();

		switch (message.getType()) {
		case RESPONSE:
		case RESPONSE_CANCEL:
		case RESPONSE_NOT_IMPLEMENTED:
		case RESPONSE_FAILED:
		    handleResponse(message);
		    break;
		case REQUEST:
		case REQUEST_CANCEL:
		    handleRequest(message);
		    break;
		case DISCONNECT:
		    // TODO: Disconnect
		    logger.severe("We do not yet handle disconnect");
		    break;
		default:
		    logger.warning("No valid message type received! (Don't know how to handle "
			    + message.getType());
		    break;
		}
	    } catch (SocketException e) {
		// Do nothing - assume the socket was closed
	    } catch (Exception e) {
		logger.severe("Exception in read buffer: " + e.toString());
	    }
	}
    }

    /**
     * @return RpcMessage read back from olad.
     * 
     * @throws Exception
     */
    private RpcMessage readMessage() throws Exception {
	byte[] header = new byte[4];
	bis.read(header);

	int headerValue = ByteBuffer.wrap(header)
		.order(ByteOrder.nativeOrder()).getInt();
	int size = headerValue & SIZE_MASK;

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
