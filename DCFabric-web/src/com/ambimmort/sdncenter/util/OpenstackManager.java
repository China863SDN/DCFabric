/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import com.ambimmort.sdncenter.util.RestClient;

import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class OpenstackManager {
    private static OpenstackManager manager;
    private List<String> controller;
    public  static String contextPath="";
    private static String tokenId = "";
    private final static String TOKENURL=":35357/v2.0/tokens";
    private final static String NETWORKSURL=":9696/v2.0/networks";
    private final static String SUBNETSURL=":9696/v2.0/subnets";
    private final static String PORTSURL=":9696/v2.0/ports";
    private static final String bangRoutePortUrl = "/gn/fabric/external/json";
    private static final String UPDATEEXTERNALURL = "/gn/fabric/external/update/json";
    private static final String GETALLCONFIG = "/gn/config/getall/json";
    private static final String SETALLCONFIG = "/gn/config/setall/json";
    private static Map<String,List<String>> externalBandInfo;
    private static List<ExternalNet> externalNets;
    private OpenstackManager() {
        this.controller = new ArrayList<String>();
    }
    
    public JSONArray getControllers() {
        return JSONArray.fromObject(controller);
    }
    
    public void addController(String ip) throws IOException {
        controller.add(ip);
        saveController();
    }
    
    public void removeController(String ip) throws IOException {
        for(int i=0;i<controller.size();i++){
            if (ip.equals(controller.get(i))) {
                controller.remove(i);
                break;
            }
        }
        saveController();
    }
    
    
   
    public static OpenstackManager getInstance() {
        if (manager == null) {
            manager = new OpenstackManager();
            try {
                manager.loadController();
            } catch (Exception ex) {
                Logger.getLogger(OpenstackManager.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        return manager;
    }
    private void loadController(){
    	String propertiesString = "";
    	if(null==propertiesString || "".equals(propertiesString)){
    		Properties pps = new Properties();
        	try {
    			pps.load(new FileInputStream(contextPath+"phy-route.properties"));
    			propertiesString = pps.getProperty("openstack-controller");
    		} catch (FileNotFoundException e) {
    			e.printStackTrace();
    		} catch (IOException e) {
    			e.printStackTrace();
    		}
        	if(!controller.contains(propertiesString)){
        		controller.add(propertiesString);
        	}
        	String[] tmp = propertiesString.split(",");
        	for(String s :tmp){
        		if(!"".equals(s)&&!controller.contains(s)){
            		controller.add(s);
            	}
        	}
    	}
    }
    private void saveController(){
    	if(null!=controller && controller.size()>0){
    		Properties pps = new Properties();
    		FileOutputStream oFile=null;
        	try {
    			pps.load(new FileInputStream(contextPath+"phy-route.properties"));
    			oFile = new FileOutputStream(contextPath+"phy-route.properties");
    			String s="";
    			for(int i=0;i<controller.size();i++){
    			  s+=controller.get(i);
    			  s+=",";
    			}
    			if(s.length()>1){
    				s = s.substring(0,s.length()-1);
    			}
    			pps.setProperty("openstack-controller", s);
    			pps.store(oFile, null);
        	} catch (FileNotFoundException e) {
    			e.printStackTrace();
    		} catch (IOException e) {
    			e.printStackTrace();
    		}finally{
    			try{
    				if(null!=oFile){
    					oFile.close();
    				}
    			}catch(Exception e){
    				e.printStackTrace();
    			}
    		}
        	
    	}
    }
    
    private void getNewToken() throws IOException{
    	if(null==controller || controller.size()==0){
    		return;
    	}
    	String params= "{\"auth\": {\"tenantName\": \"admin\",\"passwordCredentials\": {\"username\":\"admin\",\"password\":\"admin\"}}}";
    	Map<String,String> header = new HashMap<String,String>();
    	header.put("Content-Type", "application/json");
    	String resp = RestClient.getInstance().post("http://" + controller.get(0)  +TOKENURL, params,header);
    	JSONObject o = JSONObject.fromObject(resp);
        Iterator it = o.keys();
        while (it.hasNext()) {
        	String key = (String) it.next();
        	JSONObject oo = (JSONObject)o.get(key);
        	JSONObject ob = (JSONObject)oo.get("token");
        	tokenId = ob.getString("id");
        	System.out.println("tokenId update:"+tokenId+"  expired time:"+ob.getString("expires"));
        }
        
    }
    public JSONArray getAllNetwork() throws IOException{
    	if(null==controller || controller.size()==0){
    		return null;
    	}
    	Map<String,String> header = new HashMap<String,String>();
    	getNewToken();
    	header.put("X-Auth-Token", tokenId);
    	String resp = RestClient.getInstance().get("http://" + controller.get(0)+NETWORKSURL,  header);
    	JSONObject o = JSONObject.fromObject(resp);
    	Iterator it = o.keys();
    	JSONArray networks = new JSONArray();
        while (it.hasNext()) {
        	String key = (String) it.next();
        	networks = o.getJSONArray(key);
        }
        return networks;
    }
    public JSONArray getAllSubnets() throws IOException{
    	if(null==controller || controller.size()==0){
    		return null;
    	}
    	Map<String,String> header = new HashMap<String,String>();
    	getNewToken();
    	header.put("X-Auth-Token", tokenId);
    	String resp = RestClient.getInstance().get("http://" + controller.get(0)+SUBNETSURL,  header);
    	JSONObject o = JSONObject.fromObject(resp);
    	Iterator it = o.keys();
    	JSONArray subnets = new JSONArray();
        while (it.hasNext()) {
        	String key = (String) it.next();
        	subnets = o.getJSONArray(key);
        }
        return subnets;
    }
    public JSONArray getAllPorts() throws IOException{
    	if(null==controller || controller.size()==0){
    		return null;
    	}
    	Map<String,String> header = new HashMap<String,String>();
    	getNewToken();
    	header.put("X-Auth-Token", tokenId);
    	String resp = RestClient.getInstance().get("http://" + controller.get(0)+PORTSURL,  header);
    	JSONObject o = JSONObject.fromObject(resp);
    	Iterator it = o.keys();
    	JSONArray subnets = new JSONArray();
        while (it.hasNext()) {
        	String key = (String) it.next();
        	subnets = o.getJSONArray(key);
        }
        return subnets;
    }
    public List getAllExternal() throws Exception{
    	if(null==controller || controller.size()==0){
    		return null;
    	}
    	Map<String,String> header = new HashMap<String,String>();
    	Map<String,String> macs = getMacProperties();
    	getNewToken();
    	header.put("X-Auth-Token", tokenId);
    	String resp = RestClient.getInstance().get("http://" + controller.get(0)+NETWORKSURL,  header);
    	JSONObject o = JSONObject.fromObject(resp);
    	Iterator it = o.keys();
    	if(null==externalNets){
    		externalNets =  new ArrayList<ExternalNet>();
    	}else{
    		externalNets.clear();
    	}
        while (it.hasNext()) {
        	String key = (String) it.next();
        	JSONArray  jb = o.getJSONArray(key);
        	Iterator netit = jb.iterator();
        	while(netit.hasNext()){
        		JSONObject jbb = (JSONObject)netit.next();
        		if(!jbb.getString("router:external").equals("true")){
        			continue;
        		}
        		ExternalNet en = new ExternalNet();
            	en.setNetworkid(jbb.getString("id"));
            	JSONArray allsubnet = jbb.getJSONArray("subnets");
            	Iterator itt = allsubnet.iterator();
            	if(itt.hasNext()){
            		String subnetid = (String)itt.next();
            		en.setSubnetid(subnetid);
            	}
            	externalNets.add(en);
        	}	
        }
        for(ExternalNet en :externalNets ){
        	resp = RestClient.getInstance().get("http://" + controller.get(0)+SUBNETSURL+"/"+en.getSubnetid(), header);
        	o = JSONObject.fromObject(resp);
            it = o.keys();
            if(it.hasNext()){
            	String key = (String) it.next();
            	JSONObject jb = o.getJSONObject(key);
            	String gateway_ip = jb.getString("gateway_ip");
            	String cidr = jb.getString("cidr");
            	en.setGatwayip(gateway_ip);
            	en.setGatewaymac(macs.get(gateway_ip));
            	en.setCidr(cidr);
            }
        }
        resp = RestClient.getInstance().get("http://" + controller.get(0)+PORTSURL, header);
        o = JSONObject.fromObject(resp);
        it = o.keys();
		while(it.hasNext()){
			String key = (String) it.next();
        	JSONArray allport = o.getJSONArray(key);
			Iterator itt = allport.iterator();
			while(itt.hasNext()){
				JSONObject jb =(JSONObject)itt.next();
				String check_network_id = jb.getString("network_id");
				String device_owner =  jb.getString("device_owner");
				if(device_owner.equals("network:router_gateway")){
					for(ExternalNet en : externalNets){
						if(en.getNetworkid().equals(check_network_id)){
							JSONArray fixed_ip = jb.getJSONArray("fixed_ips");
							String mac = jb.getString("mac_address");
							en.setMac(mac);
							Iterator ittt = fixed_ip.iterator();
							while(ittt.hasNext()){
								JSONObject jbb =(JSONObject)ittt.next();
								String outer_interface = jbb.getString("ip_address");
								en.setOuter_interface_ip(outer_interface);
								if(null!=externalBandInfo && null!=externalBandInfo.get(outer_interface)){
									en.setBandDpid(externalBandInfo.get(outer_interface).get(0));
									en.setBandPort(externalBandInfo.get(outer_interface).get(1));
								}
							}
						}
					}
				}
			}
		}
        return externalNets;
    }
    private Map<String, String> getMacProperties() {
    	Map<String,String> macs = new HashMap<String,String>();
    	Properties pps = new Properties();
    	try {
			pps.load(new FileInputStream(contextPath+"phy-route.properties"));
			macs.put("192.168.50.129",pps.getProperty("192.168.50.129"));
			macs.put("192.168.52.129",pps.getProperty("192.168.52.129"));
			macs.put("192.168.53.129",pps.getProperty("192.168.53.129"));
			macs.put("192.168.50.1",pps.getProperty("192.168.50.1"));
			macs.put("192.168.52.1",pps.getProperty("192.168.52.1"));
			macs.put("192.168.53.1",pps.getProperty("192.168.53.1"));
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
    	return macs;
	}
 
    public String updateConfig(String dcfabricip,String port, String ipflow, 
    		String fabricon, String openstackon, String physupport, 
    		String maxswitch, String maxbuff, String maxlength)throws Exception{
    	JSONObject obj = new JSONObject();
    	obj.put("ipflow", ipflow);
    	obj.put("fabricon", fabricon);
    	obj.put("openstackon", openstackon);
    	obj.put("physupport", physupport);
    	obj.put("maxswitch", maxswitch);
    	obj.put("maxbuff", maxbuff);
    	obj.put("maxlength", maxlength);
    	System.out.println(obj.toString());
    	String resp = RestClient.getInstance().post("http://" + dcfabricip + ":" + port + SETALLCONFIG,obj.toString());
    	if (resp != null) {
            JSONObject objj = JSONObject.fromObject(resp);
            if(objj.getInt("retCode")==0){
            	return "success";
            }
        }
		return "failed";
    }
    public JSONObject searchConfig(String dcfabricip,String port)throws Exception{
    	String resp = RestClient.getInstance().get("http://" + dcfabricip + ":" + port + GETALLCONFIG );
    	JSONObject o = JSONObject.fromObject(resp);
    	JSONObject obj = new JSONObject();
    	obj.put("ip_match_flows", o.getString("ip_match_flows"));
    	obj.put("auto_fabric", o.getString("auto_fabric"));
    	obj.put("openvstack_on", o.getString("openvstack_on"));
    	obj.put("use_phy", o.getString("use_phy"));
    	obj.put("max_switch", o.getString("max_switch"));
    	obj.put("buff_num", o.getString("buff_num"));
    	obj.put("buff_len", o.getString("buff_len"));
    	return obj;
    }
	public String bandPort(String dcfabricip,String port,String dpid,String bandport,String outip) throws Exception{
    	if(null==externalNets){
    		return " data is empty";
    	}
    	if(null==externalBandInfo ){
    		externalBandInfo = new HashMap<String,List<String>>();
    	}
    	if(null== externalBandInfo.get(outip)){
    		List<String> bandInfo = new ArrayList<String>();
    		bandInfo.add(dpid);
    		bandInfo.add(bandport);
    		externalBandInfo.put(outip, bandInfo);
    	}else{
    		externalBandInfo.get(outip).clear();
    		externalBandInfo.get(outip).add(dpid);
    		externalBandInfo.get(outip).add(bandport);
    	}
    	for(ExternalNet en : externalNets){
    		if(en.getOuter_interface_ip().equals(outip)){
    			en.setBandDpid(dpid);
    			en.setBandPort(bandport);
    			String resp = RestClient.getInstance().post("http://" + dcfabricip + ":" + port + bangRoutePortUrl,JSONObject.fromObject(en).toString() );
    			if (resp != null) {
    	            JSONObject obj = JSONObject.fromObject(resp);
    	            if(obj.getInt("retCode")==0){
    	            	return "success";
    	            }
    	        }
    			return "failed";
    		}
    	}
    	return "failed";
    }
	
	public String  updateExternal(String dcfabricip,String port)  throws Exception{
		String resp = RestClient.getInstance().post("http://" + dcfabricip + ":" + port + UPDATEEXTERNALURL,"{}");
		if (resp != null) {
            JSONObject obj = JSONObject.fromObject(resp);
            if(obj.getInt("retCode")==0){
            	return "success";
            }
        }
		return "failed";
	}
}
