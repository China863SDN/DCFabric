/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.service;

import com.ambimmort.sdncenter.util.RestClient;
import com.ambimmort.sdncenter.util.StatusCode;
import java.io.IOException;
import java.util.Iterator;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class RenantManageService {

    public static final String RENANT_URL = "/gn/tenant/json";
    public static final String RENANT_MEMBER_URL = "/gn/tenantmember/json";

    public void createRenant(String ip, String port, String renantName) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_URL);
        JSONObject ob = new JSONObject();
        ob.put("tenant-name", renantName);
        String resps = RestClient.getInstance().post(sb.toString(), ob.toString());
        if (resps != null) {
            ob = JSONObject.fromObject(resps);
//            if (ob.getInt("error-code") != 0) {
//                throw getException(ob.getInt("error-code"));
//            }
            if (ob.getInt("retCode") != StatusCode.SUCCESS) {
                throw new Exception(ob.getString("retMsg"));
            }
            return;
        }
        throw new Exception("未获取到响应");
    }

    public void deleteRenant(String ip, String port, String delRenantId) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_URL);
        JSONObject ob = new JSONObject();
        ob.put("tenant-id", delRenantId);
        String resps = RestClient.getInstance().delete(sb.toString(), ob.toString());
        if (resps != null) {
            ob = JSONObject.fromObject(resps);
//            if (ob.getInt("error-code") != 0) {
//                throw getException(ob.getInt("error-code"));
//            }
            if (ob.getInt("retCode") != StatusCode.SUCCESS) {
                throw new Exception(ob.getString("retMsg"));
            }
            return;
        }
        throw new Exception("未获取到响应");
    }

    public JSONArray searchRenant(String ip, String port) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_URL);
        String resps = RestClient.getInstance().get(sb.toString());
        if (resps != null) {
            JSONObject ob = JSONObject.fromObject(resps);
//            if (ob.has("error-code") && ob.getInt("error-code") != 0) {
//                throw getException(ob.getInt("error-code"));
//            }
            if (ob.getInt("retCode") != StatusCode.SUCCESS) {
                throw new Exception(ob.getString("retMsg"));
            }
            JSONArray arr = ob.getJSONArray("tenants");
            Iterator<JSONObject> it = arr.iterator();
            while (it.hasNext()) {
                JSONObject obj = it.next();
                obj.put("cip", ip);
                obj.put("cport", port);
            }
            return arr;
        }
        throw new Exception("未获取到响应");
    }

    private Exception getException(int aInt) {
        Exception e = new Exception("未知错误！");
        switch (aInt) {
            case -1:
                e = new Exception("系统繁忙");
                break;
            case 1:
                e = new Exception("租户已存在");
                break;
            case 2:
                e = new Exception("租户不存在");
                break;
            case 3:
                e = new Exception("租户状态异常");
                break;
            case 4:
                e = new Exception("租户成员已存在");
                break;
            case 5:
                e = new Exception("租户成员不存在");
                break;
            case 6:
                e = new Exception("租户成员状态异常");
                break;
        }
        return e;
    }

    public JSONObject queryTenantMember(String ip, String port, String tenant_id) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_MEMBER_URL).append("&tenant-id=").append(tenant_id);
        String resps = RestClient.getInstance().get(sb.toString());
        if (resps != null) {
            JSONObject obj = JSONObject.fromObject(resps);
            if (obj.has("error-code") && obj.getInt("error-code") != 0) {
                throw getException(obj.getInt("error-code"));
            }
//            if (obj.getInt("retCode") != StatusCode.SUCCESS) {
//                throw new Exception(obj.getString("retMsg"));
//            }
            JSONArray arr = obj.getJSONArray("members");
            Iterator<JSONObject> it = arr.iterator();
            while (it.hasNext()) {
                JSONObject member = it.next();
                member.put("cip", ip);
                member.put("cport", port);
            }
            return obj;
        }
        throw new Exception("未获取到响应");
    }

    public void deleteTenantMember(String ip, String port, String tenant_id, String mac) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_MEMBER_URL);
        JSONObject ob = new JSONObject();
        ob.put("tenant-id", tenant_id);
        ob.put("mac", mac);
        String resps = RestClient.getInstance().delete(sb.toString(), ob.toString());
        if (resps != null) {
            ob = JSONObject.fromObject(resps);
//            if (ob.getInt("error-code") != 0) {
//                throw getException(ob.getInt("error-code"));
//            }
            if (ob.getInt("retCode") != StatusCode.SUCCESS) {
                throw new Exception(ob.getString("retMsg"));
            }
            return;
        }
        throw new Exception("未获取到响应");
    }

    public void addTenantMember(String ip, String port, String tenant_id, String mac) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(ip).append(':').append(port).append(RENANT_MEMBER_URL);
        JSONObject ob = new JSONObject();
        ob.put("tenant-id", tenant_id);
        ob.put("mac", mac);
        String resps = RestClient.getInstance().post(sb.toString(), ob.toString());
        if (resps != null) {
            ob = JSONObject.fromObject(resps);
            if (ob.getInt("error-code") != 0) {
                throw getException(ob.getInt("error-code"));
            }
//            if (ob.getInt("retCode") != StatusCode.SUCCESS) {
//                throw new Exception(ob.getString("retMsg"));
//            }
            return;
        }
        throw new Exception("未获取到响应");
    }

}
