/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.servlet;

import com.ambimmort.sdncenter.util.ExternalNet;
import com.ambimmort.sdncenter.util.OpenstackManager;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
@WebServlet(name = "OpenstackServlet", urlPatterns = {"/openstack/*"})
public class OpenstackServlet extends HttpServlet {

    /**
     * Processes requests for both HTTP
     * <code>GET</code> and
     * <code>POST</code> methods.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    protected void processRequest(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        response.setContentType("text/plain;charset=UTF-8");
        PrintWriter out = response.getWriter();

        String action = request.getPathInfo();
        String ip = request.getParameter("ip");
        if(null==OpenstackManager.contextPath || "".equals(OpenstackManager.contextPath)){
    		OpenstackManager.contextPath = request.getSession().getServletContext().getRealPath("/"); 
        }
        try {
            if ("/add".equals(action)) {
                OpenstackManager.getInstance().addController(ip);
                printInfo(0, true, out);
            } else if ("/delController".equals(action)) {
            	String openstackip = request.getParameter("controller");
            	OpenstackManager.getInstance().removeController(openstackip);
                printInfo(0, true, out);
            }else if("/allnetwork".equals(action)){
            	JSONArray o = OpenstackManager.getInstance().getAllNetwork();
            	printInfo(0, o, out);
        	}else if("/allsubnets".equals(action)){
            	JSONArray o = OpenstackManager.getInstance().getAllSubnets();
            	printInfo(0, o, out);
        	}else if("/allports".equals(action)){
            	JSONArray o = OpenstackManager.getInstance().getAllPorts();
            	printInfo(0, o, out);
        	}else if("/allExternal".equals(action)){
            	List<ExternalNet> o = OpenstackManager.getInstance().getAllExternal();
            	printInfo(0, o, out);
        	}else if("/updateExternal".equals(action)){
        		String dcfabricip = request.getParameter("ip");
        		String port = request.getParameter("port");
        		String result = OpenstackManager.getInstance().updateExternal(dcfabricip,port);
            	printInfo(0, result, out);
        	}else if("/updateConfig".equals(action)){
        		String dcfabricip = request.getParameter("ip");
        		String port = request.getParameter("port");
        		String ipflow = request.getParameter("ipflow");
        		String fabricon = request.getParameter("fabricon");
        		String openstackon = request.getParameter("openstackon");
        		String physupport = request.getParameter("physupport");
        		String maxswitch = request.getParameter("maxswitch");
        		String maxbuff = request.getParameter("maxbuff");
        		String maxlength = request.getParameter("maxlength");
        		String result = OpenstackManager.getInstance().updateConfig(dcfabricip,port,ipflow,fabricon,openstackon,physupport,maxswitch,maxbuff,maxlength);
            	printInfo(0, result, out);
        	}else if("/searchConfig".equals(action)){
        		String dcfabricip = request.getParameter("ip");
        		String port = request.getParameter("port");
        		JSONObject result = OpenstackManager.getInstance().searchConfig(dcfabricip,port);
        		System.out.println(result);
            	printInfo(0, result, out);
        	}else if("/bandPort".equals(action)){
        		String dcfabricip = request.getParameter("ip");
        		String port = request.getParameter("port");
        		String dpid = request.getParameter("dpid");
        	    String bandport = request.getParameter("bandport");
        	    String outip = request.getParameter("outip");
            	String result = OpenstackManager.getInstance().bandPort(dcfabricip,port,dpid,bandport,outip);
            	printInfo(0, result, out);
        	}else {
                JSONArray controllers = OpenstackManager.getInstance().getControllers();
                printInfo(0, controllers, out);
            }
        } catch (Exception ex) {
            Logger.getLogger(OpenstackServlet.class.getName()).log(Level.SEVERE, null, ex);
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

    // <editor-fold defaultstate="collapsed" desc="HttpServlet methods. Click on the + sign on the left to edit the code.">
    /**
     * Handles the HTTP
     * <code>GET</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
    }

    /**
     * Handles the HTTP
     * <code>POST</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
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
