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
public class MeterAndGroupTableService {

    private static final String METER_URI = "/gn/meter/json";       //操作单METER表 URL
    private static final String ALL_METERS_URI = "/gn/meters/json";   // 操作所有METER表 URI
    private static final String GROUP_URI = "/gn/group/json";
    private static final String ALL_GROUP_URI = "/gn/groups/json";
//    private static final String[] GROUP_ITEM = new String[]{"switchid", "group-id", "type", "weight", "watch_port", "watch_group", "outport"};
    private static final String[] GROUP_ITEM = new String[]{"DPID", "groupId", "type", "weight", "watchPort", "watchGroup", "output"};
//    private static final String[] METER_ITEM = new String[]{"switchid", "meter-id", "flags", "rate"};
    private static final String[] METER_ITEM = new String[]{"DPID", "meterId", "flags", "type", "precLevel", "rate", "burstSize"};

    public boolean addMeter(String ip, String port, String content) throws IOException, Exception {
        JSONObject o = JSONObject.fromObject(content);
        for (String key : METER_ITEM) {
            if ("".equals(o.get(key))) {
                o.remove(key);
            }
        }
        String resp = RestClient.getInstance().post("http://" + ip + ":" + port + METER_URI, o.toString());
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }

    public JSONArray queryMeterTable(String cip, String cport) throws IOException, Exception {
        JSONArray rs = new JSONArray();

        String host = "http://" + cip + ":" + cport;
        String resp = RestClient.getInstance().get(host + ALL_METERS_URI);
        System.out.println(resp);
        JSONObject meterObj = JSONObject.fromObject(resp);
        if (meterObj.getInt("retCode") != StatusCode.SUCCESS) {
            throw new Exception(meterObj.getString("retMsg"));
        }
        JSONArray meter_Obj = meterObj.getJSONArray("switchMeterEntries");
        Iterator it = meter_Obj.iterator();
        while (it.hasNext()) {
            JSONObject ob = (JSONObject) it.next();
            Iterator pit = ob.getJSONArray("meterEntries").iterator();
            while (pit.hasNext()) {
                JSONObject o = (JSONObject) pit.next();
                o.put("dpid", ob.getString("DPID"));
                o.put("cip", cip);
                o.put("cport", cport);
                rs.add(o);
            }
        }
        return rs;
    }
    
    public boolean removeMeterTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().delete("http://" + cip + ":" + cport + METER_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }
    
    public boolean editMeterTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().put("http://" + cip + ":" + cport + METER_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }
    
    public boolean clearMeter(String cip, String cport, String switchid) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(cip).append(":").append(cport).append(ALL_METERS_URI);
        JSONObject obj = new JSONObject();
        obj.put("DPID", switchid);
        String resp = RestClient.getInstance().delete(sb.toString(), obj.toString());
        if (resp != null) {
            JSONObject rsObj = JSONObject.fromObject(resp);
            return rsObj.getInt("retCode") == 0;
        }
        return false;
    }
    
    public boolean addGroup(String ip, String port, String content) throws IOException {
        String resp = RestClient.getInstance().post("http://" + ip + ":" + port + GROUP_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }
    
    public JSONArray queryGroupTable(String cip, String cport) throws IOException, Exception {
        JSONArray rs = new JSONArray();

        String host = "http://" + cip + ":" + cport;
        String resp = RestClient.getInstance().get(host + ALL_GROUP_URI);
        System.out.println(resp);
        JSONObject meterObj = JSONObject.fromObject(resp);
        if (meterObj.getInt("retCode") != StatusCode.SUCCESS) {
            throw new Exception(meterObj.getString("retMsg"));
        }
        JSONArray meter_Obj = meterObj.getJSONArray("switchGroups");
        Iterator it = meter_Obj.iterator();
        while (it.hasNext()) {
            JSONObject ob = (JSONObject) it.next();
            Iterator pit = ob.getJSONArray("groups").iterator();
            while (pit.hasNext()) {
                JSONObject o = (JSONObject) pit.next();
                    o.put("dpid", ob.getString("DPID"));
                    o.put("cip", cip);
                    o.put("cport", cport);
                    rs.add(o);             
            }
        }
        return rs;
    }
    
    public boolean removeGroupTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().delete("http://" + cip + ":" + cport + GROUP_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }
    
    public boolean clearGroup(String cip, String cport, String switchid) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(cip).append(":").append(cport).append(ALL_GROUP_URI);
        JSONObject obj = new JSONObject();
        obj.put("DPID", switchid);
        String resp = RestClient.getInstance().delete(sb.toString(), obj.toString());
        System.out.println(resp);
        if (resp != null) {
            JSONObject rsObj = JSONObject.fromObject(resp);
            return rsObj.getInt("retCode") == 0;
        }
        return false;
    }
}
