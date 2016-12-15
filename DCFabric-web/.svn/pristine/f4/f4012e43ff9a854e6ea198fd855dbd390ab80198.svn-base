/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.servlet;

import com.ambimmort.sdncenter.service.FlowTableService;
import com.ambimmort.sdncenter.util.ControllerManager;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Iterator;
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
@WebServlet(name = "FlowTableServlet", urlPatterns = {"/flowtable/*"})
public class FlowTableServlet extends HttpServlet {

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
        response.setContentType("text/html;charset=UTF-8");
        PrintWriter out = response.getWriter();
        
        String action = request.getPathInfo();
        String cip = request.getParameter("ip");
        String cport = request.getParameter("port");
        try {
            if ("/add".equals(action)) {
                String content = request.getParameter("content");
                boolean flag = new FlowTableService().addFlowTable(cip, cport, content);
                printInfo(0, flag, out);
            } else if ("/search".equals(action)) {
//                int type = Integer.parseInt(request.getParameter("type"));
                JSONArray arr = new JSONArray();
                FlowTableService service = new FlowTableService();
                if ("all".equals(cip)) {
                    Iterator it = ControllerManager.getInstance().getControllers().iterator();
                    while (it.hasNext()) {
                        JSONObject controller = (JSONObject)it.next();
//                        arr.addAll(service.queryFlowTable(controller.getString("ip"), controller.getString("port"), type));
                        arr.addAll(service.queryFlowTable(controller.getString("ip"), controller.getString("port")));
                    }
                } else {
//                    arr = service.queryFlowTable(cip, cport, type);
                    arr = service.queryFlowTable(cip, cport);
                }
                printInfo(0, arr, out);
            } else if ("/del".equals(action)) {
                String content = request.getParameter("content");
                boolean flag = new FlowTableService().removeFlowTable(cip, cport, content);
                printInfo(0, flag, out);
            } else if ("/edit".equals(action)) {
                String content = request.getParameter("content");
                boolean flag = new FlowTableService().editFlowTable(cip, cport, content);
                printInfo(0, flag, out);
            } else if ("/cleanflow".equals(action)) {
                String content = request.getParameter("switchid");
                boolean flag = new FlowTableService().clearFlow(cip, cport, content);
                printInfo(0, flag, out);
            }
        } catch (Exception ex) {
            Logger.getLogger(FlowTableServlet.class.getName()).log(Level.SEVERE, null, ex);
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
