// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/bind.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/public/interfaces/view_manager/view_manager.mojom.h"

namespace mojo {
namespace examples {

// ViewManagerInit is responsible for establishing the initial connection to
// the view manager. When established it loads |mojo_aura_demo|.
class ViewManagerInit : public ApplicationDelegate {
 public:
  ViewManagerInit() {}
  virtual ~ViewManagerInit() {}

  virtual void Initialize(ApplicationImpl* app) MOJO_OVERRIDE {
    app->ConnectToService("mojo:mojo_view_manager", &view_manager_init_);
    ServiceProviderPtr sp;
    view_manager_init_->Embed("mojo:mojo_aura_demo", sp.Pass(),
                              base::Bind(&ViewManagerInit::DidConnect,
                                         base::Unretained(this)));
  }

 private:
  void DidConnect(bool result) {
    DCHECK(result);
    VLOG(1) << "ViewManagerInit::DidConnection result=" << result;
  }

  ViewManagerInitServicePtr view_manager_init_;

  DISALLOW_COPY_AND_ASSIGN(ViewManagerInit);
};

}  // namespace examples

// static
ApplicationDelegate* ApplicationDelegate::Create() {
  return new examples::ViewManagerInit();
}

}  // namespace mojo
