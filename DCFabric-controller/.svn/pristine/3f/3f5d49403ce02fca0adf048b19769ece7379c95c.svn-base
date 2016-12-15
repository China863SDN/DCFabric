/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.service;

import com.ambimmort.sdncenter.util.RestClient;
import java.io.IOException;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class DcfFlowTableService {
    
    private static final String ONE_SWITCH_FLOWENTRIES = "/dcf/get/flowentries/json";
    private static final String ALL_SWITCH_FLOWENTRIES = "/dcf/get/all/flowentries/json";
    
    public JSONArray getFlowTable(String cip, String cport, String dpid) throws IOException, Exception {
    	
        String url = "http://" + cip + ":" + cport + ONE_SWITCH_FLOWENTRIES + "/" + dpid;
        String flowInfo = RestClient.getInstance().get(url);
        JSONArray rs = JSONObject.fromObject(flowInfo).getJSONArray("switchFlowEntries");
        
        return rs;
    }
    
    public JSONArray getAllFlowTable(String cip, String cport) throws IOException, Exception {
    	
        String url = "http://" + cip + ":" + cport + ALL_SWITCH_FLOWENTRIES;
        String flowInfo = RestClient.getInstance().get(url);
        JSONArray rs = JSONObject.fromObject(flowInfo).getJSONArray("switchFlowEntries");
        
        return rs;
    }
}
