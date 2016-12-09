/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.service;

import com.ambimmort.sdncenter.util.Host;
import com.ambimmort.sdncenter.util.RestClient;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.http.HttpEntity;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.util.EntityUtils;

import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class DcfLbaasService {
	private static final Logger logger = Logger.getLogger(DcfLbaasService.class.getName());
	
	private static String TOKEN_CONTENT = "{\"auth\": {\"tenantName\": \"admin\",\"passwordCredentials\": {\"username\":\"admin\",\"password\":\"admin\"}}}";
	private String m_tokenId;
	private String m_tenentId;
	
	/**
	 * 获取openstack token id
	 * @param openStackIP
	 * @param keystonePort
	 * @throws IOException
	 */
    public void getTokenId(String openStackIP, String keystonePort) throws IOException {
    	String token_url = "http://" + openStackIP + ":" + keystonePort + "/v2.0/tokens";
    	
    	String res = RestClient.getInstance().post(token_url, TOKEN_CONTENT);
    	m_tokenId = JSONObject.fromObject(res)
    		.getJSONObject("access")
    		.getJSONObject("token")
    		.getString("id");
    	m_tenentId = JSONObject.fromObject(res)
        		.getJSONObject("access")
        		.getJSONObject("token")
        		.getJSONObject("tenant")
        		.getString("id");
    }
    
    
    
    /**
     * 根据主机ID获取主机IP地址
     * @param openStackIP
     * @param novaPort
     * @param hostId
     * @return
     * @throws IOException
     */
    public String getHostAddr(String openStackIP, String novaPort, String hostId) throws IOException {
    	String url = "http://" + openStackIP + ":" + novaPort + "/v2/" + m_tenentId + "/servers/" + hostId;
    	Map<String, String> headers = new HashMap<String, String>();
        headers.put("X-Auth-Token", m_tokenId);
        
        String res = RestClient.getInstance().get(url, headers);
		Pattern pattern = Pattern.compile("\"addr\": \"(\\d+.\\d+.\\d+.\\d+)\",");
		Matcher matcher = pattern.matcher(res);
		String ret = "";
		if (matcher.find()) {
			ret = matcher.group(1);
		}

        return ret;
    }
    
    
    
    /**
     * 添加主机到lbaas
     * @param host
     * @param openStackIP
     * @param neutronPort
     * @param hostList
     * @return
     */
    public List<String> addHostToLbaasPool(Host host, String openStackIP, String neutronPort, List<String> hostList) {
    	String url = "http://" + openStackIP + ":" + neutronPort + "/v2.0/lb/members";
    	String content = "";
    	Map<String, String> headers = new HashMap<String, String>();
        headers.put("X-Auth-Token", m_tokenId);
        
        List<String> retList = new ArrayList<String>();
    	for (String ip : hostList) {
    		try {
		    	content = "{\"member\": {\"pool_id\":\"" 
		    			+ host.getPoolId()
		    			+ "\",\"address\": \"" 
		    			+ ip
		    			+ "\",\"admin_state_up\": true,\"protocol_port\": \""
		    			+ host.getProtocolPort()
		    			+ "\",\"weight\": \""
		    			+ host.getWeight()
		    			+ "\"}}";

		    	CloseableHttpResponse resp = RestClient.getInstance().post2(url, content, headers);
	        	int code = resp.getStatusLine().getStatusCode();
	        	if (201 == code) {
	        		retList.add(ip);
	        	} else {
	        		logger.log(Level.SEVERE, resp.toString());
	        	}
    		} catch (Exception e) {
    			logger.log(Level.SEVERE, e.toString());
    		}
    	}
    	
    	return retList;
    }
    
    
    
    /**
     * 创建主机实例
     * @param host
     * @param openStackIP
     * @param novaPort
     * @return
     * @throws IOException
     */
    public List<String> createHost(Host host, String openStackIP, String novaPort) throws IOException {
    	List<String> retList = new ArrayList<String>();
    	String servers_url = "http://" + openStackIP + ":" + novaPort + "/v2/" + m_tenentId + "/servers";
    	String servers_content = "{\"server\": {\"name\": \"host\",\"imageRef\":\""
    			+ host.getImageRef() 
    			+ "\",\"flavorRef\":\""
    			+ host.getFlavorRef();
    	
    	if (!host.getNetworkId().isEmpty() && !host.getSubnetId().isEmpty()) {
    		servers_content = servers_content 
    				+ "\",\"networks\":[{\"uuid\":\"" 
    				+ host.getNetworkId() 
    				+ "\",\"subnets\":[\"" 
    				+ host.getSubnetId() 
    				+ "\"]}]"
    				+ "}}";
    		
    	} else if (!host.getNetworkId().isEmpty()) {
    		servers_content = servers_content 
    				+ "\",\"networks\":[{\"uuid\":\"" 
    				+ host.getNetworkId() 
    				+ "\"}]"
    				+ "}}";
    	} else {
    		servers_content += "\"}}";
    	}
    	
    	Map<String, String> headers = new HashMap<String, String>();
        headers.put("X-Auth-Token", m_tokenId);
        int count = Integer.parseInt(host.getCount());
        
        try {
	        for (int i = 0; i < count; i++) {
	        	CloseableHttpResponse resp = RestClient.getInstance().post2(servers_url, servers_content, headers);

	        	String respStr = null;
	            HttpEntity respEntity = resp.getEntity();
	            if (respEntity != null) {
	                respStr = EntityUtils.toString(respEntity, "utf-8");
	            }
	            
	            int code = resp.getStatusLine().getStatusCode();
	        	if (202 != code) {
	        		logger.log(Level.SEVERE, respStr);
	        		continue;
	        	}

	        	String hostId = JSONObject.fromObject(respStr).getJSONObject("server").getString("id");
	        	String ipAddr = getHostAddr(openStackIP, novaPort, hostId);
	        	for ( int t = 0; t < 5 && ipAddr.isEmpty(); t++) {
	        		try {
						Thread.sleep(2000);
						ipAddr = getHostAddr(openStackIP, novaPort, hostId);
					} catch (InterruptedException e) {
						logger.log(Level.SEVERE, e.toString());
					}
	        	}
	        	
	        	if (!ipAddr.isEmpty()) {
	        		retList.add(ipAddr);
	        	}
	        }
        } catch (Exception e) {
        	logger.log(Level.SEVERE, e.toString());
        	throw new IOException("Create host instance failure!");
        }
        
        return retList;
    }
    
    
    /**
     * 刷新控制器中的lbaas pool members
     * @param controllerIp
     * @param controllerPort
     */
    public void reloadMembers(String controllerIp, String controllerPort) {
    	String url = "http://" + controllerIp + ":" + controllerPort + "/dcf/debug/loadbalance/reload";
    	String content = "{}";
        try {
			RestClient.getInstance().post(url, content);
		} catch (IOException e) {
			e.printStackTrace();
		}
    }
    
    
    
    
    /**
     * 添加主机
     * @param list
     * @param openStackIP
     * @param novaPort
     * @param neutronPort
     * @param keystonePort
     * @return
     */
    public JSONArray addHostsToLbaas(
    		List<Host> list, 
    		String openStackIP, 
    		String novaPort, 
    		String neutronPort, 
    		String keystonePort,
    		String controllerIp, 
    		String controllerPort) throws IOException {
    	JSONArray rs = new JSONArray();
    	
    	//get tokenid and tenent id
    	getTokenId(openStackIP, keystonePort);
    	
		//create hosts and add host to pool
    	List<String> ipList = null;
    	for (Host hh : list) {
    		try {
	    		ipList = null;
	    		ipList = createHost(hh, openStackIP, novaPort);
	    		ipList = addHostToLbaasPool(hh, openStackIP, neutronPort, ipList);
    		} catch (IOException e) {
    			e.printStackTrace();
    			reloadMembers(controllerIp, controllerPort);
    			logger.log(Level.SEVERE, e.toString());
    			throw e;
    		}
    		
    		for (String ip : ipList) {
    			JSONObject jj = new JSONObject();
    			jj.put("ip", ip);
    			rs.add(jj);
    		}
    	}
        
    	reloadMembers(controllerIp, controllerPort);
        return rs;
    }
    
    
    
    /**
     * 获取lbaas pool中的成员
     * @param controllerIp
     * @param controllerPort
     * @param poolId
     * @return
     * @throws IOException
     */
    public JSONObject getMembers(String controllerIp, String controllerPort, String poolId) throws IOException {
    	
    	String url = "http://" + controllerIp + ":" + controllerPort + "/dcf/debug/loadbalance/pool";
        String res = RestClient.getInstance().get(url);
        JSONArray poolList = JSONObject.fromObject(res).getJSONArray("pool_list");
        JSONObject jsonPool = null;
        for (Object pool : poolList) {
        	JSONObject o = JSONObject.fromObject(pool);
        	if (o.getString("pool_id").equals(poolId)) {
        		jsonPool = o;
        		break;
        	}
        }
        
        if (null == jsonPool) {
        	throw new IOException("pool_id is invalid!");
        }
        
        return jsonPool;
    }
    
    
    
    /**
     * 从lbaas中移除若干个主机
     * @param poolId
     * @param count
     * @param openStackIP
     * @param neutronPort
     * @param keystonePort
     * @param controllerIp
     * @param controllerPort
     * @return
     * @throws IOException
     */
    public JSONArray removeHostsFromLbaas(
    		String poolId, 
    		String count, 
    		String openStackIP, 
    		String neutronPort, 
    		String keystonePort, 
    		String controllerIp, 
    		String controllerPort) throws IOException {
    	JSONArray rs = new JSONArray();
    	//get token id
    	getTokenId(openStackIP, keystonePort);
    	Map<String, String> headers = new HashMap<String, String>();
        headers.put("X-Auth-Token", m_tokenId);
        
    	//get pool member list
    	JSONObject jsonPool = getMembers(controllerIp, controllerPort, poolId);
        Map<String, String> mm = new TreeMap<String , String>(new Sort());
        Map<String, String> ipMap = new HashMap<String, String>();
        JSONArray memberList = jsonPool.getJSONArray("member");
        for (Object mem : memberList) {
        	JSONObject ob = JSONObject.fromObject(mem);
        	mm.put(ob.getString("connect"), ob.getString("member_id"));
        	ipMap.put(ob.getString("member_id"), ob.getString("member_ip"));
        }
        
        //remove member from pool
        int index = 0;
        int number = Integer.parseInt(count);
        String url = "";
        String urlMem = "http://" + openStackIP + ":" + neutronPort + "/v2.0/lb/members/";
        List<String> ipList = new ArrayList<String>();
        for (Map.Entry<String, String> en : mm.entrySet()) {
        	if (index < number) {
	        	url = urlMem + en.getValue();
	        	try {
	        		RestClient.getInstance().delete(url, headers);
	        		ipList.add(ipMap.get(en.getValue()));
	        	} catch (Exception e) {
	        		logger.log(Level.SEVERE, e.toString());
	        	}
	        	
	        	index++;
        	} else {
        		break;
        	}
        }
        
		for (String ip : ipList) {
			JSONObject jj = new JSONObject();
			jj.put("ip", ip);
			rs.add(jj);
		}
    	
		reloadMembers(controllerIp, controllerPort);
    	return rs;
    }
    
    
    
    
    /**
     * 仅作为功能测试使用，获取一个默认的Host配置（随机选取）
     * @param openStackIP
     * @param novaPort
     * @param neutronPort
     * @param keystonePort
     * @return
     */
    public Host getDefaultHost(String openStackIP, String novaPort, String neutronPort, String keystonePort) {
    	
    	Host host = new Host();
    	
    	try {
	    	getTokenId(openStackIP, keystonePort);
	    	Map<String, String> headers = new HashMap<String, String>();
	        headers.put("X-Auth-Token", m_tokenId);
	        
	    	//get image
	    	String url = "http://" + openStackIP + ":" + "9292" + "/v1/images";
	    	String res = RestClient.getInstance().get(url, headers);
	    	JSONArray array = JSONObject.fromObject(res).getJSONArray("images");
	    	if (array.size() > 0) {
	    		host.setImageRef(((JSONObject)array.get(0)).getString("id"));
	    	}
	    	
	    	//get flavor
	    	url = "http://" + openStackIP + ":" + novaPort + "/v2/" + m_tenentId + "/flavors";
	    	res = RestClient.getInstance().get(url, headers);
	    	array = JSONObject.fromObject(res).getJSONArray("flavors");
	    	if (array.size() > 0) {
	    		host.setFlavorRef(((JSONObject)array.get(0)).getString("id"));
	    	}
	    	
	    	//get pool id
	    	url = "http://" + openStackIP + ":" + neutronPort + "/v2.0/lb/pools";
	    	res = RestClient.getInstance().get(url, headers);
	    	array = JSONObject.fromObject(res).getJSONArray("pools");
	    	if (array.size() > 0) {
	    		host.setPoolId(((JSONObject)array.get(0)).getString("id"));
	    	}
	    	
	    	//get network id
	    	url = "http://" + openStackIP + ":" + neutronPort + "/v2.0/networks";
	    	res = RestClient.getInstance().get(url, headers);
	    	array = JSONObject.fromObject(res).getJSONArray("networks");
	    	if (array.size() > 0) {
	    		host.setNetworkId(((JSONObject)array.get(0)).getString("id"));
	    	}
    	} catch (Exception e) {
    		logger.log(Level.SEVERE, e.toString());
    	}
    	
    	return host;
    }
    
    
    
    
    private class Sort implements Comparator<String>
    {
	    public int compare(String o1, String o2)
	    {
	    	if (Integer.parseInt(o1) < Integer.parseInt(o2)) {
	    		return -1;
	    	} else {
	    		return 1;
	    	}
	    }
    }
}
