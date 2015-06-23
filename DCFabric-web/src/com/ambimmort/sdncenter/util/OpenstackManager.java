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
}
