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
public class SwitchService {
    
//    private static final String SWITCH_INFO_URI = "/gn/core/switchinfo/json";
    private static final String SWITCH_PORT_INFO = "/gn/switchinfo/json";
    
    public JSONArray getSwitchInfo(String cip, String cport) throws IOException, Exception {
        JSONArray rs = new JSONArray();

        String host = "http://" + cip + ":" + cport;
        String switch_info = RestClient.getInstance().get(host + SWITCH_PORT_INFO);
        JSONObject switchObj = JSONObject.fromObject(switch_info);
        if (switchObj.getInt("retCode") != StatusCode.SUCCESS) {
            throw new Exception(switchObj.getString("retMsg"));
        }
        JSONArray switch_Obj = switchObj.getJSONArray("switchInfo");

        Iterator it = switch_Obj.iterator();
        while (it.hasNext()) {
            JSONObject ob = (JSONObject) it.next();
            
            JSONObject newObj = new JSONObject();
            newObj.put("dpid", ob.getString("DPID"));
            String[] inetAddress = ob.getString("inetAddr").split(":");
            newObj.put("ip", inetAddress[0]);
            newObj.put("port", inetAddress[1]);
            newObj.put("manufacturer", ob.getString("mfrDesc"));
            newObj.put("hardware", ob.getString("hwDesc"));
            newObj.put("software", ob.getString("swDesc"));
            newObj.put("openflow", ob.getString("openflow"));
            
            Iterator pit = ob.getJSONArray("ports").iterator();
            JSONArray netport = new JSONArray();
            while (pit.hasNext()) {
                JSONObject o = (JSONObject) pit.next();
                
                JSONObject no = new JSONObject();
                no.put("name", o.getString("name"));
                no.put("hardwareAddress", o.getString("hwAddr"));
                no.put("portNumber", o.getString("portNo"));
                netport.add(no);
            }
            newObj.put("netport", netport);

            rs.add(newObj);      
        }
        return rs;
    }
}
