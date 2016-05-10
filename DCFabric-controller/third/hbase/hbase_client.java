import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
import org.apache.hadoop.hbase.filter.SingleColumnValueFilter;

@SuppressWarnings("unused")
public class hbase_client {
        private static Configuration conf = null;
        private static String SPLIT = "|";

        public static void Init_conf(String ip, String port) throws Exception {
                String ipport = ip + ":" + port;

                conf = HBaseConfiguration.create();
                if (conf == null) {
                    System.out.println("Init hbase configuration error!");
                }
                conf.set("hbase.master", ipport);
                conf.set("hbase.zookeeper.quorum", ip);

                System.out.println("Init hbase configuration ok!");

        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static void createTable(String tableName, String cloumnFamily)
                        throws Exception {
                HBaseAdmin admin = new HBaseAdmin(conf);
                if (admin.tableExists(tableName)) {
                        System.out.println("The table [" + tableName + "] already exists!");
                } else {
                        HTableDescriptor tableDesc = new HTableDescriptor(tableName);
                        tableDesc.addFamily(new HColumnDescriptor(cloumnFamily));
                        admin.createTable(tableDesc);
                        System.out.println("Create table [" + tableName + "] ok!");
                }
        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static void addRecord(String tableName, String columnFamily,
                        String rowkey, String column, String value) throws Exception {
                try {
                        HTable table = new HTable(conf, tableName);
                        Put put = new Put(Bytes.toBytes(rowkey));

                        put.add(Bytes.toBytes(columnFamily), Bytes.toBytes(column),
                                        Bytes.toBytes(value));
                        table.put(put);
                } catch (Exception e) {
                        System.out.println("Add record failed, table[" + tableName
                                        + "], column family[" + columnFamily + "], row key["
                                        + rowkey + "], column[" + column + "], value[" + value
                                        + "] , Exception message: " + e.getLocalizedMessage());
                }
        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static void deleteRecord(String tableName, String rowkey)
                        throws Exception {
                try {
                        HTable table = new HTable(conf, tableName);
                        List list = new ArrayList();
                        Delete del = new Delete(rowkey.getBytes());
                        list.add(del);
                        table.delete(list);
                } catch (Exception e) {
                        System.out.println("Delete record failed, table[" + tableName
                                        + "], row key[" + rowkey + "], Exception message: "
                                        + e.getLocalizedMessage());
                }
        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static String getOneRecord(String tableName, String rowkey)
                        throws Exception {
                try {
                        String str = "";

                        HTable table = new HTable(conf, tableName);
                        Get get = new Get(rowkey.getBytes());
                        Result rs = table.get(get);
                        for (KeyValue kv : rs.raw()) {	
                                str += new String(kv.getQualifier()) + SPLIT;
                                str += new String(kv.getValue()) + SPLIT;
                        }

                        return str;
                } catch (Exception e) {
                        System.out.println("Get record by row failed, table[" + tableName
                                        + "], row key[" + rowkey + "] Exception message: "
                                        + e.getLocalizedMessage());
                        return null;
                }
        }
				
		@SuppressWarnings({ "rawtypes", "unchecked" })
		public static String getOneCell(String tableName, String rowkey, String cloumnFamily, String cloumn)
						throws Exception {
				try {
						String str = "";

						HTable table = new HTable(conf, tableName);
						Get get = new Get(rowkey.getBytes());
						get.addColumn(cloumnFamily.getBytes(), cloumn.getBytes()); 
						Result rs = table.get(get);
						for (KeyValue kv : rs.raw()) {		
								str = new String(kv.getValue());
						}
						return str;
				} catch (Exception e) {
                        System.out.println("Get cell by cloumn failed, table[" + tableName
                                        + "], row key[" + rowkey + "] cloumnFamily[" + cloumnFamily
                                        + "], cloumn[" + cloumn + "]Exception message: "
                                        + e.getLocalizedMessage());
                        return null;
				}
		}

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static String[] getRecordsByFilter(String tableName, String columnFamily,
                        String column, String value) throws Exception {
                try {
//                        System.out.println("tableName=" + tableName + " ,columnFamily="
//                                        + columnFamily + ", column=" + column + ",value=" + value);
                        String str = null;
                        ArrayList list = new ArrayList();
                        HTable table = new HTable(conf, tableName);
                        Scan s = new Scan();
                        SingleColumnValueFilter filter = new SingleColumnValueFilter(
                                        Bytes.toBytes(columnFamily), Bytes.toBytes(column),
                                        CompareOp.EQUAL, Bytes.toBytes(value));
                        s.setFilter(filter);
                        ResultScanner ss = table.getScanner(s);
                        for (Result r : ss) {
                                str = "";
                                for (KeyValue kv : r.raw()) {
                                        str += new String(kv.getQualifier()) + SPLIT;
                                        str += new String(kv.getValue()) + SPLIT;
                                }
//                                System.out.println(str);
                                list.add(str);
                        }

                        String[] rRecord = (String[]) list.toArray(new String[0]);
                        return rRecord;
                } catch (Exception e) {
                        System.out.println("Get record by filter failed, table[" + tableName
                                        + "], column family[" + columnFamily + "], column["
                                        + column + "], value[" + value + "], Exception message: "
                                        + e.getLocalizedMessage());
                        return null;
                }
        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static String[] getAllRecords(String tableName, String columnFamily) throws Exception {
                try {
//                        System.out.println("tableName=" + tableName + " ,columnFamily=" + columnFamily);
                        String str = null;
                        ArrayList list = new ArrayList();
                        HTable table = new HTable(conf, tableName);
                        Scan s = new Scan();
                        ResultScanner ss = table.getScanner(s);
                        for (Result r : ss) {
                                str = "";
                                for (KeyValue kv : r.raw()) {
                                        str += new String(kv.getQualifier()) + SPLIT;
                                        str += new String(kv.getValue()) + SPLIT;
                                }

//                                System.out.println(str);
                                list.add(str);
                        }

                        String[] rRecord = (String[]) list.toArray(new String[0]);
                        return rRecord;
                } catch (Exception e) {
                        System.out.println("Get record by filter failed, table[" + tableName
                                        + "], column family[" + columnFamily + "], Exception message: "
                                        + e.getLocalizedMessage());
                        return null;
                }
        }

        @SuppressWarnings({ "rawtypes", "unchecked" })
        public static void delTable(String tableName) throws Exception {
                try {
                        HBaseAdmin admin = new HBaseAdmin(conf);
                        admin.disableTable(tableName);
                        admin.deleteTable(tableName);
                } catch (Exception e) {
                        System.out.println("Delete table failed, table[" + tableName
                                        + "], Exception message: " + e.getLocalizedMessage());
                }
        }

}
