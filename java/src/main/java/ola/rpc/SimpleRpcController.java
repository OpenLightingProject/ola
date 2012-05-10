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

import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcController;

/**
 * Simple Rpc Controller implementation.
 */
public class SimpleRpcController implements RpcController {

    private boolean failed = false;
    private boolean cancelled = false;
    private String error = null;
    private RpcCallback<Object> callback = null;

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#errorText()
     */
    public String errorText() {
        return error;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#failed()
     */
    public boolean failed() {
        return failed;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#isCanceled()
     */
    public boolean isCanceled() {
        return cancelled;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#notifyOnCancel(com.google.protobuf.RpcCallback)
     */
    public void notifyOnCancel(RpcCallback<Object> notifyCallback) {
        callback = notifyCallback;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#reset()
     */
    public void reset() {
        failed = false;
        cancelled = false;
        error = null;
        callback = null;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#setFailed(java.lang.String)
     */
    public void setFailed(String reason) {
        failed = true;
        error = reason;
    }

    /* (non-Javadoc)
     * @see com.google.protobuf.RpcController#startCancel()
     */
    public void startCancel() {
        cancelled = true;
        callback.run(null);
    }
}
