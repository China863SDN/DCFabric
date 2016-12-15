/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.service;

import com.ambimmort.sdncenter.util.RestClient;
import java.io.IOException;
import java.util.Iterator;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class FirewallManageService {
    public static final String STATUS_URL = "/gn/firewall/status/json";
    public static final String FIREWALL_ON_URL = "/gn/firewall/enable/json";
    public static final String FIREWALL_OFF_URL = "/gn/firewall/disable/json";
    public static final String RULE_URL = "/gn/firewall/rules/json";

    public String queryFirewallStatus(String ip, String port) throws IOException {
        String resp = RestClient.getInstance().get("http://" + ip + ":" + port + STATUS_URL);
        JSONObject rs = JSONObject.fromObject(resp);
        if (rs.has("result") && rs.getString("result").indexOf("enabled") > -1) {
            return "ON";
        }
        return "OFF";
    }

    public boolean firewallONorOFF(String ip, String port, String action) throws IOException {
        String resp = null;
        if ("ON".equals(action)) {
            resp = RestClient.getInstance().get("http://" + ip + ":" + port + FIREWALL_ON_URL);
        } else {
            resp = RestClient.getInstance().get("http://" + ip + ":" + port + FIREWALL_OFF_URL);
        }
        JSONObject rs = JSONObject.fromObject(resp);
        if ("success".equals(rs.getString("status"))) {
            return true;
        }
        return false;
    }

    public JSONArray searchFirewallRegular(String ip, String port) throws IOException {
        String resp = RestClient.getInstance().get("http://" + ip + ":" + port + RULE_URL);
        JSONArray rs = JSONArray.fromObject(resp);
        Iterator<JSONObject> it = rs.iterator();
        while (it.hasNext()) {
            JSONObject o = it.next();
            o.put("cip", ip);
            o.put("cport", port);
        }
        return rs;
    }

    public boolean addRegular(String ip, String port, String content) throws IOException {
        String resp = RestClient.getInstance().post("http://" + ip + ":" + port + RULE_URL, content);
        return true;
    }

    public boolean delRegular(String ip, String port, String rid) throws IOException {
        JSONObject o = new JSONObject();
        o.put("ruleid", rid);
        String resp = RestClient.getInstance().delete("http://" + ip + ":" + port + RULE_URL, o.toString());
        return true;
    }
    
}
