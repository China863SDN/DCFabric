/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ambimmort.sdncenter.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.http.HttpEntity;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.util.EntityUtils;

/**
 *
 * @author Administrator
 */
public class RestClient {
    private static RestClient rc;
    
    public static RestClient getInstance(){
        if (rc == null) {
            rc = new RestClient();
        }
        return rc;
    }
    
    public String post(String uri, String content) throws IOException {
        CloseableHttpClient client = HttpClients.createDefault();
        HttpPost post = new HttpPost(uri);
        StringEntity entity = new StringEntity(content, ContentType.create("text/json", "utf-8"));
        post.setEntity(entity);
        CloseableHttpResponse resp = client.execute(post);
        int code = resp.getStatusLine().getStatusCode();
        if (code == 200) {
            HttpEntity respEntity = resp.getEntity();
            
            String respStr = null;
            if (respEntity != null) {
                respStr = EntityUtils.toString(respEntity, "utf-8");
                Logger.getLogger(RestClient.class.getName()).log(Level.INFO, "RECEIVE: {0}", respStr);
            }
            return respStr;
        }
        throw new IOException("POST请求失败！");
    }
    
    public String put(String uri, String content) throws IOException {
        CloseableHttpClient client = HttpClients.createDefault();
        HttpPut put = new HttpPut(uri);
        StringEntity entity = new StringEntity(content, ContentType.create("text/json", "utf-8"));
        put.setEntity(entity);
        CloseableHttpResponse resp = client.execute(put);
        int code = resp.getStatusLine().getStatusCode();
        if (code == 200) {
            HttpEntity respEntity = resp.getEntity();
            
            String respStr = null;
            if (respEntity != null) {
                respStr = EntityUtils.toString(respEntity, "utf-8");
                Logger.getLogger(RestClient.class.getName()).log(Level.INFO, "RECEIVE: {0}", respStr);
            }
            return respStr;
        }
        throw new IOException("PUT请求失败");
    }
    
    public String delete(String uri, String content) throws IOException {
        CloseableHttpClient client = HttpClients.createDefault();
        RestDeleteMethod post = new RestDeleteMethod(uri);
        StringEntity entity = new StringEntity(content, ContentType.create("text/json", "utf-8"));
        post.setEntity(entity);
        CloseableHttpResponse resp = client.execute(post);
        int code = resp.getStatusLine().getStatusCode();
        if (code == 200) {
            HttpEntity respEntity = resp.getEntity();
            String respStr = null;
            if (respEntity != null) {
                respStr = EntityUtils.toString(respEntity, "utf-8");
                Logger.getLogger(RestClient.class.getName()).log(Level.INFO, "RECEIVE: {0}", respStr);
            }
            return respStr;
        }
        throw new IOException("POST请求失败！");
    }
    
    public String get(String uri) throws IOException {
        CloseableHttpClient client = HttpClients.createDefault();
        HttpGet post = new HttpGet(uri);
        CloseableHttpResponse resp = client.execute(post);
        int code = resp.getStatusLine().getStatusCode();
        if (code == 200) {
            HttpEntity respEntity = resp.getEntity();
            
            String respStr = null;
            if (respEntity != null) {
                respStr = EntityUtils.toString(respEntity, "utf-8");
                Logger.getLogger(RestClient.class.getName()).log(Level.INFO, "RECEIVE: {0}", respStr);
            }
            return respStr;
        }
        throw new IOException("POST请求失败！");
    }
    
}
