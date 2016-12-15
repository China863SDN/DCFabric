/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Administrator
 */
public class Config {
    
    private static Config conf;
    
    public static Config getInstance() {
        try {
            if (conf == null) {
                conf = new Config();
            }
        } catch (IOException ex) {
            Logger.getLogger(Config.class.getName()).log(Level.SEVERE, null, ex);
        }
        return conf;
    }
    
    private Properties prop;
    
    private Config() throws IOException{
        prop = new Properties();
        InputStream is = Config.class.getClassLoader().getResourceAsStream("config.properties");
        prop.load(is);
        is.close();
    }
    
    public String get(String key) {
        return (String)prop.get(key);
    }
}
