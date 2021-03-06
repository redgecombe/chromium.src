// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.bindings;

import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.mojo.MojoTestCase;
import org.chromium.mojo.bindings.BindingsTestUtils.CapturingErrorHandler;
import org.chromium.mojo.bindings.BindingsTestUtils.RecordingMessageReceiver;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.Handle;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoResult;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;

import java.nio.ByteBuffer;
import java.util.ArrayList;

/**
 * Testing the {@link Connector} class.
 */
public class ConnectorTest extends MojoTestCase {

    private static final long RUN_LOOP_TIMEOUT_MS = 25;

    private static final int DATA_LENGTH = 1024;

    private MessagePipeHandle mHandle;
    private Connector mConnector;
    private MessageWithHeader mTestMessage;
    private RecordingMessageReceiver mReceiver;
    private CapturingErrorHandler mErrorHandler;

    /**
     * @see MojoTestCase#setUp()
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        Core core = CoreImpl.getInstance();
        Pair<MessagePipeHandle, MessagePipeHandle> handles = core.createMessagePipe(
                new MessagePipeHandle.CreateOptions());
        mHandle = handles.first;
        mConnector = new Connector(handles.second);
        mReceiver = new RecordingMessageReceiver();
        mConnector.setIncomingMessageReceiver(mReceiver);
        mErrorHandler = new CapturingErrorHandler();
        mConnector.setErrorHandler(mErrorHandler);
        mConnector.start();
        mTestMessage = BindingsTestUtils.newRandomMessageWithHeader(DATA_LENGTH);
        assertNull(mErrorHandler.exception);
        assertEquals(0, mReceiver.messages.size());
    }

    /**
     * @see MojoTestCase#tearDown()
     */
    @Override
    protected void tearDown() throws Exception {
        mConnector.close();
        mHandle.close();
        super.tearDown();
    }

    /**
     * Test sending a message through a {@link Connector}.
     */
    @SmallTest
    public void testSendingMessage() {
        mConnector.accept(mTestMessage);
        assertNull(mErrorHandler.exception);
        ByteBuffer received = ByteBuffer.allocateDirect(DATA_LENGTH);
        MessagePipeHandle.ReadMessageResult result = mHandle.readMessage(received, 0,
                MessagePipeHandle.ReadFlags.NONE);
        assertEquals(MojoResult.OK, result.getMojoResult());
        assertEquals(DATA_LENGTH, result.getMessageSize());
        assertEquals(mTestMessage.getMessage().buffer, received);
    }

    /**
     * Test receiving a message through a {@link Connector}
     */
    @SmallTest
    public void testReceivingMessage() {
        mHandle.writeMessage(mTestMessage.getMessage().buffer, new ArrayList<Handle>(),
                MessagePipeHandle.WriteFlags.NONE);
        nativeRunLoop(RUN_LOOP_TIMEOUT_MS);
        assertNull(mErrorHandler.exception);
        assertEquals(1, mReceiver.messages.size());
        MessageWithHeader received = mReceiver.messages.get(0);
        assertEquals(0, received.getMessage().handles.size());
        assertEquals(mTestMessage.getMessage().buffer, received.getMessage().buffer);
    }

    /**
     * Test receiving an error through a {@link Connector}.
     */
    @SmallTest
    public void testErrors() {
        mHandle.close();
        nativeRunLoop(RUN_LOOP_TIMEOUT_MS);
        assertNotNull(mErrorHandler.exception);
        assertEquals(MojoResult.FAILED_PRECONDITION, mErrorHandler.exception.getMojoResult());
    }
}
