/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.servlet;

import com.ambimmort.sdncenter.service.RenantManageService;
import java.io.IOException;
import java.io.PrintWriter;
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
@WebServlet(name = "RenantServlet", urlPatterns = {"/renant/*"})
public class RenantServlet extends HttpServlet {

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
        response.setContentType("text/json;charset=UTF-8");
        PrintWriter out = response.getWriter();
        
        String action = request.getPathInfo();
        String ip = request.getParameter("ip");
        String port = request.getParameter("port");
        try {
            RenantManageService service = new RenantManageService();
            if ("/addrenant".equals(action)) {
                String renantName = request.getParameter("content");
                service.createRenant(ip, port, renantName);
                printInfo(0, "操作成功", out);
            } else if ("/delrenant".equals(action)) {
                String delRenantId = request.getParameter("content");
                service.deleteRenant(ip, port, delRenantId);
                printInfo(0, "操作成功", out);
            } else if ("/searchrenant".equals(action)) {
                JSONArray renants = service.searchRenant(ip, port);
                printInfo(0, renants, out);
            } else if ("/addtenantmember".equals(action)) {
                String tenant_id = request.getParameter("tenantid");
                String mac = request.getParameter("mac");
                service.addTenantMember(ip, port, tenant_id, mac);
                printInfo(0, "操作成功", out);
            } else if ("/deltenantmember".equals(action)) {
                String tenant_id = request.getParameter("tenantid");
                String mac = request.getParameter("mac");
                service.deleteTenantMember(ip, port, tenant_id, mac);
                printInfo(0, "操作成功", out);
            } else if ("/searchtenantmember".equals(action)) {
                String tenant_id = request.getParameter("tenantid");
                JSONObject rs = service.queryTenantMember(ip, port, tenant_id);
                printInfo(0, rs, out);
            }
        } catch (Exception ex) {
            Logger.getLogger(this.getClass().getName()).log(Level.SEVERE, null, ex);
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
