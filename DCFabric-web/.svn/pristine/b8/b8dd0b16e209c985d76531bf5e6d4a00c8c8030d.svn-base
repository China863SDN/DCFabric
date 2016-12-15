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
public class GateWayService {
    
    private static final String SUBNET_URI = "/gn/subnet/json";
    private static final String SUBNET_LIST = "/gn/subnet/list/all/json";
    
    public void addGateWay(String cip, String cport, String name, String address) throws IOException, Exception {
        JSONObject o = new JSONObject();
        o.put("name", name);
        o.put("subnet", address);
        
        RestClient.getInstance().post("http://" + cip + ":" + cport + SUBNET_URI, o.toString());
//        JSONObject rObj = JSONObject.fromObject(result);
//        if (rObj.getInt("retCode") != StatusCode.SUCCESS) {
//            throw new Exception(rObj.getString("retMsg"));
//        }
    }
    
    public void removeGateWay(String cip, String cport, String name) throws IOException, Exception  {
        JSONObject o = new JSONObject();
        o.put("name", name);
        
        RestClient.getInstance().delete("http://" + cip + ":" + cport + SUBNET_URI, o.toString());
//        JSONObject rObj = JSONObject.fromObject(result);
//        if (rObj.getInt("retCode") != StatusCode.SUCCESS) {
//            throw new Exception(rObj.getString("retMsg"));
//        }
    }
    
    public JSONArray gatewayList(String ip, String port) throws IOException, Exception {
        String listStr = RestClient.getInstance().get("http://" + ip + ":" + port + SUBNET_URI);
        JSONObject subConfig = JSONObject.fromObject(listStr);
        if (subConfig.getInt("retCode") != StatusCode.SUCCESS) {
            throw new Exception(subConfig.getString("retMsg"));
        }
        return subConfig.getJSONArray("subnets");
    }
}
