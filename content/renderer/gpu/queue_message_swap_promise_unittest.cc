// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/queue_message_swap_promise.h"

#include <vector>

#include "base/memory/scoped_vector.h"
#include "cc/base/swap_promise.h"
#include "content/renderer/gpu/frame_swap_message_queue.h"
#include "content/renderer/gpu/render_widget_compositor.h"
#include "content/renderer/render_widget.h"
#include "content/test/mock_render_process.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sync_message_filter.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class TestRenderWidget : public RenderWidget {
 public:
  using RenderWidget::QueueMessageImpl;

 private:
  virtual ~TestRenderWidget() {}

  DISALLOW_COPY_AND_ASSIGN(TestRenderWidget);
};

class TestSyncMessageFilter : public IPC::SyncMessageFilter {
 public:
  TestSyncMessageFilter() : IPC::SyncMessageFilter(NULL) {}

  virtual bool Send(IPC::Message* message) OVERRIDE {
    messages_.push_back(message);
    return true;
  }

  ScopedVector<IPC::Message>& messages() { return messages_; }

 private:
  virtual ~TestSyncMessageFilter() {}

  ScopedVector<IPC::Message> messages_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncMessageFilter);
};

struct QueueMessageData {
  MessageDeliveryPolicy policy;
  bool commit_requested;
  int source_frame_number;
};

class QueueMessageSwapPromiseTest : public testing::Test {
 public:
  QueueMessageSwapPromiseTest()
      : frame_swap_message_queue_(new FrameSwapMessageQueue()),
        sync_message_filter_(new TestSyncMessageFilter()) {}

  virtual ~QueueMessageSwapPromiseTest() {}

  scoped_ptr<cc::SwapPromise> QueueMessageImpl(IPC::Message* msg,
                                               MessageDeliveryPolicy policy,
                                               bool commit_requested,
                                               int source_frame_number) {
    return TestRenderWidget::QueueMessageImpl(msg,
                                              policy,
                                              frame_swap_message_queue_,
                                              sync_message_filter_,
                                              commit_requested,
                                              source_frame_number).Pass();
  }

  ScopedVector<IPC::Message>& DirectSendMessages() {
    return sync_message_filter_->messages();
  }

  ScopedVector<IPC::Message>& NextSwapMessages() {
    next_swap_messages_.clear();
    scoped_ptr<FrameSwapMessageQueue::SendMessageScope> send_message_scope =
        frame_swap_message_queue_->AcquireSendMessageScope();
    frame_swap_message_queue_->DrainMessages(&next_swap_messages_);
    return next_swap_messages_;
  }

  bool ContainsMessage(const ScopedVector<IPC::Message>& messages,
                       const IPC::Message& message) {
    if (messages.empty())
      return false;
    for (ScopedVector<IPC::Message>::const_iterator i = messages.begin();
         i != messages.end();
         ++i) {
      if ((*i)->type() == message.type())
        return true;
    }
    return false;
  }

  bool NextSwapHasMessage(const IPC::Message& message) {
    return ContainsMessage(NextSwapMessages(), message);
  }

  void QueueMessages(QueueMessageData data[], size_t count) {
    for (size_t i = 0; i < count; ++i) {
      messages_.push_back(
          IPC::Message(0, i + 1, IPC::Message::PRIORITY_NORMAL));
      promises_.push_back(
          QueueMessageImpl(new IPC::Message(messages_[i]),
                           data[i].policy,
                           data[i].commit_requested,
                           data[i].source_frame_number).release());
    }
  }

  void CleanupPromises() {
    for (ScopedVector<cc::SwapPromise>::iterator i = promises_.begin();
         i != promises_.end();
         ++i) {
      if (*i)
        (*i)->DidSwap(NULL);
    }
  }

 protected:
  void VisualStateSwapPromiseDidNotSwap(
      cc::SwapPromise::DidNotSwapReason reason);

  scoped_refptr<FrameSwapMessageQueue> frame_swap_message_queue_;
  scoped_refptr<TestSyncMessageFilter> sync_message_filter_;
  std::vector<IPC::Message> messages_;
  ScopedVector<cc::SwapPromise> promises_;

 private:
  ScopedVector<IPC::Message> next_swap_messages_;

  DISALLOW_COPY_AND_ASSIGN(QueueMessageSwapPromiseTest);
};

TEST_F(QueueMessageSwapPromiseTest, NextSwapPolicySchedulesMessageForNextSwap) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
  };
  QueueMessages(data, arraysize(data));

  ASSERT_TRUE(promises_[0]);
  EXPECT_TRUE(DirectSendMessages().empty());
  EXPECT_FALSE(frame_swap_message_queue_->Empty());
  EXPECT_TRUE(NextSwapHasMessage(messages_[0]));

  CleanupPromises();
}

TEST_F(QueueMessageSwapPromiseTest, NextSwapPolicyNeedsAtMostOnePromise) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
  };
  QueueMessages(data, arraysize(data));

  ASSERT_TRUE(promises_[0]);
  ASSERT_FALSE(promises_[1]);

  CleanupPromises();
}

TEST_F(QueueMessageSwapPromiseTest, NextSwapPolicySendsMessageOnNoUpdate) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
  };
  QueueMessages(data, arraysize(data));

  promises_[0]->DidNotSwap(cc::SwapPromise::COMMIT_NO_UPDATE);
  EXPECT_TRUE(ContainsMessage(DirectSendMessages(), messages_[0]));
  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_TRUE(frame_swap_message_queue_->Empty());
}

TEST_F(QueueMessageSwapPromiseTest, NextSwapPolicySendsMessageOnSwapFails) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
  };
  QueueMessages(data, arraysize(data));

  promises_[0]->DidNotSwap(cc::SwapPromise::SWAP_FAILS);
  EXPECT_TRUE(ContainsMessage(DirectSendMessages(), messages_[0]));
  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_TRUE(frame_swap_message_queue_->Empty());
}

TEST_F(QueueMessageSwapPromiseTest, NextSwapPolicyRetainsMessageOnCommitFails) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_NEXT_SWAP, false, 1},
  };
  QueueMessages(data, arraysize(data));

  promises_[0]->DidNotSwap(cc::SwapPromise::COMMIT_FAILS);
  EXPECT_TRUE(DirectSendMessages().empty());
  EXPECT_FALSE(frame_swap_message_queue_->Empty());
  EXPECT_TRUE(NextSwapHasMessage(messages_[0]));
}

TEST_F(QueueMessageSwapPromiseTest, VisualStateDirectSend) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, false, 1},
  };
  QueueMessages(data, arraysize(data));

  ASSERT_FALSE(promises_[0]);
  EXPECT_FALSE(DirectSendMessages().empty());
  EXPECT_TRUE(frame_swap_message_queue_->Empty());
  EXPECT_TRUE(NextSwapMessages().empty());
}

TEST_F(QueueMessageSwapPromiseTest,
       VisualStateQueuesMessageWhenCommitRequested) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, true, 1},
  };
  QueueMessages(data, arraysize(data));

  ASSERT_TRUE(promises_[0]);
  EXPECT_TRUE(DirectSendMessages().empty());
  EXPECT_FALSE(frame_swap_message_queue_->Empty());
  EXPECT_TRUE(NextSwapMessages().empty());

  CleanupPromises();
}

TEST_F(QueueMessageSwapPromiseTest,
       VisualStateQueuesMessageWhenOtherMessageAlreadyQueued) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, true, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, true, 1},
  };
  QueueMessages(data, arraysize(data));

  EXPECT_TRUE(DirectSendMessages().empty());
  EXPECT_FALSE(frame_swap_message_queue_->Empty());
  EXPECT_FALSE(NextSwapHasMessage(messages_[1]));

  CleanupPromises();
}

TEST_F(QueueMessageSwapPromiseTest, VisualStateSwapPromiseDidSwap) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, true, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, false, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, false, 2},
  };
  QueueMessages(data, arraysize(data));

  promises_[0]->DidSwap(NULL);
  ASSERT_FALSE(promises_[1]);
  ScopedVector<IPC::Message> messages;
  messages.swap(NextSwapMessages());
  EXPECT_EQ(2u, messages.size());
  EXPECT_TRUE(ContainsMessage(messages, messages_[0]));
  EXPECT_TRUE(ContainsMessage(messages, messages_[1]));
  EXPECT_FALSE(ContainsMessage(messages, messages_[2]));

  promises_[2]->DidSwap(NULL);
  messages.swap(NextSwapMessages());
  EXPECT_EQ(1u, messages.size());
  EXPECT_TRUE(ContainsMessage(messages, messages_[2]));

  EXPECT_TRUE(DirectSendMessages().empty());
  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_TRUE(frame_swap_message_queue_->Empty());
}

void QueueMessageSwapPromiseTest::VisualStateSwapPromiseDidNotSwap(
    cc::SwapPromise::DidNotSwapReason reason) {
  QueueMessageData data[] = {
    /* { policy, commit_requested, source_frame_number } */
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, true, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, false, 1},
    {MESSAGE_DELIVERY_POLICY_WITH_VISUAL_STATE, false, 2},
  };
  QueueMessages(data, arraysize(data));

  promises_[0]->DidNotSwap(reason);
  ASSERT_FALSE(promises_[1]);
  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_EQ(2u, DirectSendMessages().size());
  EXPECT_TRUE(ContainsMessage(DirectSendMessages(), messages_[0]));
  EXPECT_TRUE(ContainsMessage(DirectSendMessages(), messages_[1]));
  EXPECT_FALSE(ContainsMessage(DirectSendMessages(), messages_[2]));

  promises_[2]->DidNotSwap(reason);
  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_TRUE(ContainsMessage(DirectSendMessages(), messages_[2]));

  EXPECT_TRUE(NextSwapMessages().empty());
  EXPECT_TRUE(frame_swap_message_queue_->Empty());
}

TEST_F(QueueMessageSwapPromiseTest, VisalStateSwapPromiseDidNotSwapNoUpdate) {
  VisualStateSwapPromiseDidNotSwap(cc::SwapPromise::COMMIT_NO_UPDATE);
}

TEST_F(QueueMessageSwapPromiseTest,
       VisalStateSwapPromiseDidNotSwapCommitFails) {
  VisualStateSwapPromiseDidNotSwap(cc::SwapPromise::COMMIT_FAILS);
}

TEST_F(QueueMessageSwapPromiseTest, VisalStateSwapPromiseDidNotSwapSwapFails) {
  VisualStateSwapPromiseDidNotSwap(cc::SwapPromise::SWAP_FAILS);
}

}  // namespace content
