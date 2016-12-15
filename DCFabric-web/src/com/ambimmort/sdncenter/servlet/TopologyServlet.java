/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.servlet;

import com.ambimmort.sdncenter.service.TopologyService;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Map;
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
@WebServlet(name = "TopologyServlet", urlPatterns = {"/topology/*"})
public class TopologyServlet extends HttpServlet {

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
        if(null==TopologyService.contextPath || "".equals(TopologyService.contextPath)){
        	TopologyService.contextPath = request.getSession().getServletContext().getRealPath("/"); 
        }
        String action = request.getPathInfo();
        String ip = request.getParameter("ip");
        String port = request.getParameter("port");
        try {
            if ("/link".equals(action)) {
                JSONObject links = new TopologyService().getSwitchLink(ip, port);
                printInfo(0, links, out);
            } else if ("/host".equals(action)) {
                JSONObject hosts = new TopologyService().getSwitchHost(ip, port);
                printInfo(0, hosts, out);
            }else if("/exchange_host.json".equals(action)){
            	ArrayList<Map<String,Object>> maps = new TopologyService().getNodeData(ip, port);
            	response.setContentType("json;charset=UTF-8");
            	printInfoNew("tree",maps, out);
            } 
            
            else if("/exchange_tree.json".equals(action)){
            	ArrayList<Map<String,Object>> maps = new TopologyService().getNodeData(ip, port);
            	response.setContentType("json;charset=UTF-8");
            	printInfoNew("tree",maps, out);
            } else if("/exchange_circle.json".equals(action)){
            	ArrayList<Map<String,Object>> maps = new TopologyService().getNodeData(ip, port);
            	response.setContentType("json;charset=UTF-8");
            	printInfoNew("circle",maps, out);
            } else if("/detail_rate.json".equals(action)){
            	String srcDpid = request.getParameter("srcNodeId");
            	String dstDpid = request.getParameter("dstNodeId");
            	JSONObject  maps = new TopologyService().getCountInfo(srcDpid, dstDpid, ip, port);
            	response.setContentType("json;charset=UTF-8");
            	printInfoNewTwo(maps, out);
            } else if ("/setFabric".equals(action)) {
                JSONObject hosts = new TopologyService().setFabric(ip, port);
                printInfo(0, hosts, out);
            } else if ("/deleteFabric".equals(action)) {
                JSONObject hosts = new TopologyService().deleteFabric(ip, port);
                printInfo(0, hosts, out);
            } else if ("/path_optimal.json".equals(action)) {
            	String srcId = request.getParameter("srcId");
            	String dstId = request.getParameter("dstId");
            	JSONArray v = new TopologyService().getSingleFabric(ip, port,srcId,dstId);
            	response.setContentType("json;charset=UTF-8");
            	printInfoNewTwo( v, out);
            } else if ("/allFabric".equals(action)) {
                JSONObject hosts = new TopologyService().getAllFabric(ip, port);
                printInfo(0, hosts, out);
            } else if ("/partFabric".equals(action)) {
            	String dpidList = request.getParameter("partF");
                JSONObject hosts = new TopologyService().setPartFabric(ip, port,dpidList);
                printInfo(0, hosts, out);
            } else if ("/saveFabric".equals(action)) {
                JSONObject hosts = new TopologyService().saveFabric(ip, port);
                printInfo(0, hosts, out);
            } else if ("/interval_read.json".equals(action)) {
                String interal = new TopologyService().getInterval(ip, port);
                printInfo(0, interal, out);
            } else {
                response.setStatus(404);
            }
        } catch(Exception ex) {
            Logger.getLogger(TopologyServlet.class.getName()).log(Level.SEVERE, null, ex);
            printInfo(1,ex.getMessage(), out);
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
    	//System.out.println("current:"+new Date(System.currentTimeMillis()-1224*24*60*60*1000)+"   before:"+(System.currentTimeMillis()-1224*24*60*60*1000));        
    	JSONObject obj = new JSONObject();
        obj.put("status", status);
        obj.put("data", data);
        out.print(obj.toString());
    }
    
    private void printInfoNew(String layout,ArrayList<Map<String,Object>> map, PrintWriter out) {
    	//System.out.println("current:"+new Date(System.currentTimeMillis()-1224*24*60*60*1000)+"   before:"+(System.currentTimeMillis()-1224*24*60*60*1000));    	
    	JSONObject obj = new JSONObject();
    	 obj.put("nodes", map);
         obj.put("layout", layout);
         obj.put("need-layout", "true");
         out.print(obj);
    }
    private void printInfoNewTwo(Object o, PrintWriter out) {
    	//System.out.println("current:"+new Date(System.currentTimeMillis()-1224*24*60*60*1000)+"   before:"+(System.currentTimeMillis()-1224*24*60*60*1000));
        out.print(o);
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
