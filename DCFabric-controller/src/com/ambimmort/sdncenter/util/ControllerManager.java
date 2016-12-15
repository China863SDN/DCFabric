/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.sf.json.JSONArray;
import net.sf.json.JSONObject;

/**
 *
 * @author Administrator
 */
public class ControllerManager {
    private static ControllerManager manager;
    private static final String CONTROLLER_FILE = "/controller.txt";
    private List<JSONObject> controller;
    
    private ControllerManager() {
        this.controller = new ArrayList<JSONObject>();
    }
    
    public JSONArray getControllers() {
        return JSONArray.fromObject(controller);
    }
    
    public void addController(String ip, String port) throws IOException {
        JSONObject o = new JSONObject();
        o.put("ip", ip);
        o.put("port", port);
        controller.add(o);
        saveController();
    }
    
    public void removeController(String ip, String port) throws IOException {
        Iterator<JSONObject> it = controller.iterator();
        while (it.hasNext()) {
            JSONObject o = it.next();
            if (ip.equals(o.getString("ip"))) {
                it.remove();
                break;
            }
        }
        saveController();
    }
    
    private void loadController() throws IOException {
        String f = Config.getInstance().get("local_path") + CONTROLLER_FILE;
        BufferedReader br = new BufferedReader(new FileReader(f));
        String line = null;
        while ((line = br.readLine()) != null) {
            String[] tmp = line.split("\\|");
            JSONObject o = new JSONObject();
            o.put("ip", tmp[0]);
            o.put("port", tmp[1]);
            controller.add(o);
        }
        br.close();
    }
    
    private void saveController() throws IOException {
        String f = Config.getInstance().get("local_path") + CONTROLLER_FILE;
        File pFile = new File(f).getParentFile();
        if (!pFile.exists()) {
            pFile.mkdirs();
        }
        PrintWriter pw = new PrintWriter(new FileWriter(f));
        Iterator<JSONObject> it = controller.iterator();
        while (it.hasNext()) {
            JSONObject o = it.next();
            pw.print(o.get("ip"));
            pw.print('|');
            pw.println(o.get("port"));
        }
        pw.close();
    }
    
    public static ControllerManager getInstance() {
        if (manager == null) {
            manager = new ControllerManager();
            try {
                manager.loadController();
            } catch (IOException ex) {
                Logger.getLogger(ControllerManager.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        return manager;
    }
}
