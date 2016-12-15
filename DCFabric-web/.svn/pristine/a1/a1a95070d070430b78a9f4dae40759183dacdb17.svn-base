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
public class FlowTableService {
    //private static final String FLOW_URI = "/gn/staticflowentrypusher/json";
    private static final String FLOW_URI = "/gn/flow/json";
    private static final String[] FLOW_LIST_URI = new String[]{"/gn/staticflowentrypusher/list/all/json", "/gn/dynamicflowentrypusher/list/all/json"};
    private static final String FLOW_ALL_URI = "/gn/flows/all/json";
    private static final String FLOW_DELETE_URI = "/gn/delallflow/json";
//    private static final String[] FLOW_ITEMS = new String[]{"arp_spa","arp_tpa","arp_sha","arp_tha","mpls_lable","switchid", "table", "src-inport", "vlan-id","src-mac","dst-mac","dl-type","src-ip","dst-ip","nw-proto","tp-src","tp-dst","vlan-priority","nw-tos","priority","action", "meter-id", "group-id", "idle_timeout", "hard_timeout"};
    private static final String[] FLOW_ITEMS = new String[]{};
    
    public boolean addFlowTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().post("http://" + cip + ":" + cport + FLOW_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }
    
    private JSONArray getStaticFlowTable(String cip, String cport) throws IOException {
        String resp = RestClient.getInstance().get("http://" + cip + ":" + cport + FLOW_LIST_URI[0]);
        JSONObject obj = JSONObject.fromObject(resp);
        
        JSONArray rs = new JSONArray();
        
        Iterator it = obj.keys();
        while (it.hasNext()) {
            String dpid = (String) it.next();
            
            JSONArray fts = obj.getJSONArray(dpid);
            Iterator fit = fts.iterator();
            while (fit.hasNext()) {
                JSONObject ft = (JSONObject) fit.next();
                
                JSONObject o = new JSONObject();
                o.put("dpid", dpid);
                o.put("cip", cip);
                o.put("cport", cport);
                o.put("flow_type", 0);
                for (String key : FLOW_ITEMS) {
                    if ("action".equals(key)) {
                        Object eth_dst = ft.get("actions=set_eth_dst");
                        Object output = ft.get("actions=output");
                        StringBuilder sb = new StringBuilder();
                        if (eth_dst != null) {
                            sb.append("set_eth_dst=").append(eth_dst).append(";");
                        }
                        if (output != null) {
                            sb.append("output=").append(output);
                        }
                        o.put(key, sb.length() == 0 ? null : sb.toString());
                    } else {
                    o.put(key, ft.get(key));
                    }
                }
                rs.add(o);
            }
        }
        return rs;
    }
    
    private JSONArray getDynamicFlowTable(String cip, String cport) throws IOException {
        String resp = RestClient.getInstance().get("http://" + cip + ":" + cport + FLOW_LIST_URI[1]);
        JSONObject obj = JSONObject.fromObject(resp);
        
        JSONArray rs = new JSONArray();
        
        Iterator it = obj.keys();
        while (it.hasNext()) {
            String dpid = (String) it.next();
            
            JSONArray fts = obj.getJSONArray(dpid);
            Iterator fit = fts.iterator();
            while (fit.hasNext()) {
                JSONObject ft = (JSONObject) fit.next();
                
                JSONObject o = new JSONObject();
                o.put("dpid", dpid);
                o.put("cip", cip);
                o.put("cport", cport);
                o.put("flow_type", 1);
                for (String key : FLOW_ITEMS) {
                    if ("action".equals(key)) {
                        Object eth_dst = ft.get("actions=set_eth_dst");
                        Object output = ft.get("actions=output");
                        StringBuilder sb = new StringBuilder();
                        if (eth_dst != null) {
                            sb.append("set_eth_dst=").append(eth_dst).append(";");
                        }
                        if (output != null) {
                            sb.append("output=").append(output);
                        }
                        o.put(key, sb.length() == 0 ? null : sb.toString());
                    } else {
                    o.put(key, ft.get(key));
                    }
                }
                rs.add(o);
            }
        }
        return rs;
    }

    public JSONArray queryFlowTable(String cip, String cport) throws IOException, Exception {
        JSONArray rs = new JSONArray();
        
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(cip).append(':').append(cport).append(FLOW_ALL_URI);
        String resp = RestClient.getInstance().get(sb.toString());
        if (resp != null) {
            Iterator<JSONObject> dpidIt = JSONObject.fromObject(resp).getJSONArray("switchFlowEntries").iterator();
            while (dpidIt.hasNext()) {
                JSONObject  dpid = dpidIt.next();
                Iterator<JSONObject> flowIt = dpid.getJSONArray("flowEntries").iterator();
                while (flowIt.hasNext()) {
                    JSONObject flow = flowIt.next();
                    flow.put("dpid", dpid.get("DPID"));
                    flow.put("cip", cip);
                    flow.put("cport", cport);
                    rs.add(flow);
                }
            }
        }
        
        return rs;
    }

    public boolean removeFlowTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().delete("http://" + cip + ":" + cport + FLOW_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }

    public boolean editFlowTable(String cip, String cport, String content) throws IOException {
        String resp = RestClient.getInstance().put("http://" + cip + ":" + cport + FLOW_URI, content);
        if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            return obj.getInt("retCode") == 0;
        }
        return false;
    }

    public boolean clearFlow(String cip, String cport, String switchid) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append("http://").append(cip).append(":").append(cport).append(FLOW_ALL_URI);
        JSONObject obj = new JSONObject();
        obj.put("DPID", switchid);
        String resp = RestClient.getInstance().delete(sb.toString(), obj.toString());
        if (resp != null) {
            JSONObject rsObj = JSONObject.fromObject(resp);
            return rsObj.getInt("retCode") == 0;
        }
        return false;
    }
}
