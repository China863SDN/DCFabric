/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.servlet;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.ambimmort.sdncenter.service.DcfLbaasService;
import com.ambimmort.sdncenter.util.Host;

import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
@WebServlet(name = "DcfLbaasServlet", urlPatterns = {"/dcf/lbaas/*", "/dcf/lbaas"})
public class DcfLbaasServlet extends HttpServlet {

	private static final long serialVersionUID = -8552810564701213494L;
	private static final Logger logger = Logger.getLogger(DcfLbaasServlet.class.getName());
	
	private List<Host> hostList = new ArrayList<Host>();

	/**
     * Processes requests for HTTP
     * <code>POST</code> methods.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    protected void processPostRequest(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
    	
    	hostList.clear();
		response.setContentType("text/html;charset=UTF-8");
		PrintWriter out = response.getWriter();
		
		//body
		StringBuffer buf = new StringBuffer();
		String line = null;
		BufferedReader reader = request.getReader();
		while ((line = reader.readLine()) != null) {
			buf.append(line);
		}

		try {
			//params
			String openStackIP = request.getParameter("openstack_ip");
			if (openStackIP == null || openStackIP.isEmpty()) {
				printInfo(1, "invalid url, eg: /sdn_center/dcf/lbaas?openstack_ip=X.X.X.X&nova_port=8774&neutron_port=9696&keystone_port=35357&controller_ip=X.X.X.X&controller_port=8081", out);
				return;
			}
			
			String novaPort = request.getParameter("nova_port");
			if (novaPort == null || novaPort.isEmpty()) {
				novaPort = "8774";
			}
			
			String neutronPort = request.getParameter("neutron_port");
			if (neutronPort == null || neutronPort.isEmpty()) {
				neutronPort = "9696";
			}
			
			String keystonePort = request.getParameter("keystone_port");
			if (keystonePort == null || keystonePort.isEmpty()) {
				keystonePort = "35357";
			}
			
			String controllerIp = request.getParameter("controller_ip");
			if (controllerIp == null || controllerIp.isEmpty()) {
				printInfo(1, "invalid url, eg: /sdn_center/dcf/lbaas?openstack_ip=X.X.X.X&nova_port=8774&neutron_port=9696&keystone_port=35357&controller_ip=X.X.X.X&controller_port=8081", out);
				return;
			}
			
			String controllerPort = request.getParameter("controller_port");
			if (controllerPort == null || controllerPort.isEmpty()) {
				controllerPort = "8081";
			}
			
			String content = buf.toString();
			if (buf.length() > 0 && 0 != content.compareTo("TEST")) {
				JSONObject jsonObject = JSONObject.fromObject(content);
				JSONArray array = jsonObject.getJSONArray("hosts");
				for (Object ob : array) {
					JSONObject jsonHost = JSONObject.fromObject(ob);
					Host hh = new Host();
					if (jsonHost.containsKey("imageRef")) {
						hh.setImageRef(jsonHost.getString("imageRef"));
					}
					
					if (jsonHost.containsKey("flavorRef")) {
						hh.setFlavorRef(jsonHost.getString("flavorRef"));
					}
					
					if (jsonHost.containsKey("poolId")) {
						hh.setPoolId(jsonHost.getString("poolId"));
					}
					
					if (jsonHost.containsKey("protocolPort")) {
						hh.setProtocolPort(jsonHost.getString("protocolPort"));
					}
					
					if (jsonHost.containsKey("weight")) {
						hh.setWeight(jsonHost.getString("weight"));
					}
					
					if (jsonHost.containsKey("count")) {
						hh.setCount(jsonHost.getString("count"));
					}
					
					if (jsonHost.containsKey("networkId")) {
						hh.setNetworkId(jsonHost.getString("networkId"));
					}
					
					if (jsonHost.containsKey("subnetId")) {
						hh.setSubnetId(jsonHost.getString("subnetId"));
					}
					
					hostList.add(hh);
				}
			} else if (buf.length() <= 0) {
				printInfo(1, "Request's BODY is NULL!", out);
				return;
			}
			
			DcfLbaasService service = new DcfLbaasService();
			if (0 == hostList.size() && 0 == content.compareTo("TEST")) {
				Host hh = service.getDefaultHost(openStackIP, novaPort, neutronPort, keystonePort);
				hostList.add(hh);
				logger.log(Level.WARNING, "POST BODY is TEST, Gene a default host config: " + hh);
			}
			
			JSONArray rs = service.addHostsToLbaas(
					hostList, 
					openStackIP, 
					novaPort, 
					neutronPort, 
					keystonePort,
					controllerIp,
					controllerPort);
			
			printInfo(0, rs, out);
		} catch (Exception ex) {
			logger.log(Level.SEVERE, ex.toString());
			printInfo(1, ex.getMessage(), out);
		} finally {
			out.close();
		}
    }
    
    
	/**
     * Processes requests for HTTP
     * <code>GET</code> methods.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    protected void processDeleteRequest(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
		response.setContentType("text/html;charset=UTF-8");
		PrintWriter out = response.getWriter();
		
		try {
			
			//params
			String openStackIP = request.getParameter("openstack_ip");
			if (openStackIP == null || openStackIP.isEmpty()) {
				printInfo(1, "invalid url, eg: /sdn_center/dcf/lbaas/{pool_id}/{count}?openstack_ip=X.X.X.X&neutron_port=9696&keystone_port=35357&controller_ip=X.X.X.X&controller_port=8081", out);
				return;
			}
			
			String neutronPort = request.getParameter("neutron_port");
			if (neutronPort == null || neutronPort.isEmpty()) {
				neutronPort = "9696";
			}
			
			String keystonePort = request.getParameter("keystone_port");
			if (keystonePort == null || keystonePort.isEmpty()) {
				keystonePort = "35357";
			}
			
			String controllerIp = request.getParameter("controller_ip");
			if (controllerIp == null || controllerIp.isEmpty()) {
				printInfo(1, "invalid url, eg: /sdn_center/dcf/lbaas/{pool_id}/{count}?openstack_ip=X.X.X.X&neutron_port=9696&keystone_port=35357&controller_ip=X.X.X.X&controller_port=8081", out);
				return;
			}
			
			String controllerPort = request.getParameter("controller_port");
			if (controllerPort == null || controllerPort.isEmpty()) {
				controllerPort = "8081";
			}
			
			//pool_id and count
			String url = request.getRequestURL().toString();
			Pattern pattern = Pattern.compile("/dcf/lbaas/([\\w-]+)/(\\d+)");
			Matcher matcher = pattern.matcher(url);
			String poolId = "";
			String count = "";
			if (matcher.find()) {
				poolId = matcher.group(1);
				count = matcher.group(2);
			}
			
			if (poolId.isEmpty() || count.isEmpty()) {
				printInfo(1, "invalid url, eg: /sdn_center/dcf/lbaas/{pool_id}/{count}?openstack_ip=X.X.X.X&neutron_port=9696&keystone_port=35357&controller_ip=X.X.X.X&controller_port=8081", out);
				return;
			}
			
			JSONArray rs = new DcfLbaasService().removeHostsFromLbaas(poolId, count, openStackIP, neutronPort, keystonePort, controllerIp, controllerPort);
			printInfo(0, rs, out);
		} catch (Exception ex) {
			logger.log(Level.SEVERE, ex.toString());
			printInfo(1, ex.getMessage(), out);
		} finally {
			out.close();
		}
    }
    
    
    /**
     * 输出信息到页面
     *
     * @param status 结果状态码。0-代表成功，1-代表失败
     * @param data 结果消息。当status为0时，代表返回的结果，当status为1时，代表失败的信息
     * @param out 输出流
     */
    private void printInfo(int status, Object data, PrintWriter out) {
        JSONObject obj = new JSONObject();
        obj.put("status", status);
        obj.put("data", data);
        out.print(obj.toString());
    }
    

    @Override
	protected void doPost(HttpServletRequest req, HttpServletResponse resp)
			throws ServletException, IOException {
    	processPostRequest(req, resp);
	}


	@Override
	protected void doGet(HttpServletRequest req, HttpServletResponse resp)
			throws ServletException, IOException {
		processDeleteRequest(req, resp);
	}


	/**
     * Returns a short description of the servlet.
     *
     * @return a String containing servlet description
     */
    @Override
    public String getServletInfo() {
        return "Short description";
    }// </editor-fold>
}
