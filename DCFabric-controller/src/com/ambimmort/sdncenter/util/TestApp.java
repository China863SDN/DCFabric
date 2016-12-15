/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Administrator
 */
public class TestApp {
    public static void main(String[] args) {
        try {
            String rss = RestClient.getInstance().get("http://10.8.1.46:8081/gn/tenantmember/json?tenant-id=0");
            System.out.println(rss);
        } catch (IOException ex) {
            Logger.getLogger(TestApp.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
