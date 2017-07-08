// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_LIBCEF_BROWSER_PEPPER_PEPPER_FLASH_BROWSER_HOST_H_
#define CEF_LIBCEF_BROWSER_PEPPER_PEPPER_FLASH_BROWSER_HOST_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

namespace base {
class Time;
}

namespace content {
class BrowserPpapiHost;
class ResourceContext;
}

class GURL;

class PepperFlashBrowserHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashBrowserHost(content::BrowserPpapiHost* host,
                         PP_Instance instance,
                         PP_Resource resource);
  ~PepperFlashBrowserHost() override;

  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnUpdateActivity(ppapi::host::HostMessageContext* host_context);
  int32_t OnGetLocalTimeZoneOffset(
      ppapi::host::HostMessageContext* host_context,
      const base::Time& t);
  int32_t OnGetLocalDataRestrictions(ppapi::host::HostMessageContext* context);

  void GetLocalDataRestrictions(ppapi::host::ReplyMessageContext reply_context,
                                const GURL& document_url,
                                const GURL& plugin_url);

  content::BrowserPpapiHost* host_;
  int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashBrowserHost);
};

#endif  // CEF_LIBCEF_BROWSER_PEPPER_PEPPER_FLASH_BROWSER_HOST_H_
