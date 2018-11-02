var paths={
    getFlowTabls:"../gn/flow/json/",
    deleteTable:"../gn/flow/json/",
    flushTable:"../gn/flows/all/json/",
    addTable:"../gn/flow/json/",
    editTable:"../gn/flow/json/",
    switchInfo:"../gn/switch/json/"
}
var controller
var tableObj= $('#datatable-table').DataTable({
       
    columnDefs: [{
        orderable: false,
        className: 'select-checkbox',
        targets: 0
    }],
    "ordering": false,
    select: {
        style: 'multi',
        selector: 'td:first-child'
    },
    "bAutoWidth": false,
        "aoColumns" : [
            { sWidth: '20px' },
            { sWidth: '60px' },
            { sWidth: '60px' },
            { sWidth: 'auto' },
            { sWidth: 'auto' },
            { sWidth: 'auto' },{ sWidth: '60px' }
        ]  
})
   
tableObj.on("click", "th.select-checkbox", function() {
    if ($("th.select-checkbox").hasClass("selected")) {
        tableObj.rows().deselect();
        $("th.select-checkbox").removeClass("selected");
    } else {
        tableObj.rows().select();
        $("th.select-checkbox").addClass("selected");
    }
    ifSelected()
}).on("select deselect", function() {
    ("Some selection or deselection going on")
    if (tableObj.rows({
            selected: true
        }).count() !== tableObj.rows().count()) {
        $("th.select-checkbox").removeClass("selected");
    } else {
        $("th.select-checkbox").addClass("selected");
    }
    ifSelected()
});
function showFlowTables(data){
    tableObj.clear()
    tableObj.rows.add(data)
    tableObj.draw();
   
}

var flowTables=[]
var switchObj;
var mockdata = false;
var actionGroup = ["applyAction"]
var dataCheck
function intCheck(min,max,v){
    console.log(v)
    if(isNaN(+v)){
        return false
    }else{
        v= +v
        if(v%1!==0){
            return false
        }else{
            return v>=min &&  v<=max
        }
    }
}
function strCheck(v){
    console.log(v)
    if(v == "controller"){
        return true
    }
	else
		return false
}
var checkRules={
    inport:{
        des:'Accept value: 1-60 or 4294967293',
        isMatch:function(v){
            return intCheck(1,60,v) ||intCheck(4294967293,4294967293,v)
        }
    },
    ethDst:{
        des:"Accept MAC Addr: xx:xx:xx:xx:xx:xx",
        isMatch:function(v){
            var t= /^([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}$/
            return t.test(v)
        }
    },
    ethSrc :{
        des:"Accept MAC Addr: xx:xx:xx:xx:xx:xx",
        isMatch:function(v){
            var t= /^([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}$/
            return t.test(v)
        }
    },
    ethType:{
        des:"<br/>Accept value: ARP/IPV4/IPV6/MPLS/VLAN/UNKNOW",
        isMatch:function(v){
            var list='ARP/IPV4/IPV6/MPLS/VLAN/UNKNOW'.split("/")
            return _.indexOf(list,v)!==-1
        }
    },
    ipv4Src:{
        des:"Accept IP Addr: xx.xx.xx.xx",
        isMatch:function(v){
            var t=/^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
            return t.test(v)
        }
    },
    ipv4Dst:{ 
        des:"Accept IP Addr: xx.xx.xx.xx",
        isMatch:function(v){
            var t=/^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
            return t.test(v)
        }
    },
    ippProto:{
        des:"Accept value: TCP/UDP/ICMP",
        isMatch:function(v){
            var list='TCP/UDP/ICMP'
            return _.indexOf(list,v)!==-1
        }
    },
    tcpSrc:{
        des:"Accept value: 1-65535",
        isMatch:function(v){
            return intCheck(1,65535,v)
        }
    },
    udpSrc:{
        des:"Accept value: 1-65535",
        isMatch:function(v){
            return intCheck(1,65535,v)
        }
    },
    arpOp:{
        des:'Accept value: 1 or 2 [1-Request,2-Reply]',
        isMatch:function(v){
            return intCheck(1,2,v)
        }
    },
    arpSha:{
        des:"Accept IP Addr: xx.xx.xx.xx",
        isMatch:function(v){
            var t=/^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
            return t.test(v)
        }
    },
    arpSpa:{
        des:"Accept IP Addr: xx.xx.xx.xx",
        isMatch:function(v){
            var t=/^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
            return t.test(v)
        }
    },
    arpTpa:{
        des:"Accept MAC Addr: xx:xx:xx:xx:xx:xx",
        isMatch:function(v){
            var t= /^([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}$/
            return t.test(v)
        }
    },
    arpTha:{
        des:"Accept MAC Addr: xx:xx:xx:xx:xx:xx",
        isMatch:function(v){
            var t= /^([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}$/
            return t.test(v)
        }
    },
    output:{
        des:'Accept value: 1-65535 or 4294967293 or controller',
        isMatch:function(v){
            return intCheck(1,65535,v) ||intCheck(4294967293,4294967293,v) || strCheck(v)
        }
    },
    gotoTable:{
        des:"Accept value: 1-255",
        isMatch:function(v){
            return intCheck(0,255,v)
        }
    }



}
var flowTableConfig = {
    "DPID": "交换机 Datapath ID",
    "tableId": "表号[Default: 0]",
    "idleTimeout": "空闲超时时间[Default: 200]",
    "hardTimeout": "硬性超时时间[Default: 200]",
    "priority": "优先级",
    "match": {
        "inport": "入口",
        "ethDst": "以太网目的 MAC",
        "ethSrc": "以太网源 MAC",
        "ethType": "以太网类型",
        "vlanId": "VLAN ID",
        "vlanPcp": "VLAN 优先级",
        "ipDscp": "DSCP",
        "ipEcn": "ECN",
        "ipProto": "IP 协议类型",
        "ipv4Src": "ipv4 源 IP",
        "ipv4Dst": "ipv4 目的 IP",
        "tcpSrc": "TCP 源端口",
        "tcpDst": "TCP 目的端口",
        "udpSrc": "UDP 源端口",
        "udpDst": "UDP 目的端口",
        "arpOp": "ARP 协议类型",
        "arpSpa": "ARP 发送方 IP",
        "arpTpa": "ARP 发送方 MAC",
        "arpSha": "ARP 目的 IP",
        "arpTha": "ARP 目的 MAC",
        "mplsLabel": "MPLS 标签"
    },
    "instruction": {
        "applyAction": {
            "output": "转出端口",
            "pushVlan": "加 VLAN 标签",
            "popVlan": "去 VLAN 标签",
            "pushMpls": "加 MPLS 标签",
            "popMpls": "去 MPLS 标签",
            "group": "Group 表 ID",
            "setField": {
                "inport": "入口",
                "ethDst": "以太网目的 MAC",
                "ethSrc": "以太网源 MAC",
                "ethType": "以太网类型",
                "vlanId": "VLAN ID",
                "vlanPcp": "VLAN 优先级",
                "ipDscp": "DSCP",
                "ipEcn": "ECN",
                "ipProto": "IP 协议类型",
                "ipv4Src": "ipv4 源 IP",
                "ipv4Dst": "ipv4 目的 IP",
                "tcpSrc": "TCP 源端口",
                "tcpDst": "TCP 目的端口",
                "udpSrc": "UDP 源端口",
                "udpDst": "UDP 目的端口",
                "arpOp": "ARP 协议类型",
                "arpSpa": "ARP 发送方 IP",
                "arpTpa": "ARP 发送方 MAC",
                "arpSha": "ARP 目的 IP",
                "arpTha": "ARP 目的 MAC",
                "mplsLabel": "MPLS 标签"
            }
        },
        "writeAction": {
            "output": "转出端口",
            "pushVlan": "加 VLAN 标签",
            "popVlan": "去 VLAN 标签",
            "pushMpls": "加 MPLS 标签",
            "popMpls": "去 MPLS 标签",
            "group": "Group 表 ID",
            "setField": {
                "inport": "入口",
                "ethDst": "以太网目的 MAC",
                "ethSrc": "以太网源 MAC",
                "ethType": "以太网类型",
                "vlanId": "VLAN ID",
                "vlanPcp": "VLAN 优先级",
                "ipDscp": "DSCP",
                "ipEcn": "ECN",
                "ipProto": "IP 协议类型",
                "ipv4Src": "ipv4 源 IP",
                "ipv4Dst": "ipv4 目的 IP",
                "tcpSrc": "TCP 源端口",
                "tcpDst": "TCP 目的端口",
                "udpSrc": "UDP 源端口",
                "udpDst": "UDP 目的端口",
                "arpOp": "ARP 协议类型",
                "arpSpa": "ARP 发送方 IP",
                "arpTpa": "ARP 发送方 MAC",
                "arpSha": "ARP 目的 IP",
                "arpTha": "ARP 目的 MAC",
                "mplsLabel": "MPLS 标签"
            }
        },
        "clearAction": "表示清空所有 Action[无需赋值]",
        "gotoTable": "table ID",
        "meter": "计量表 ID"
    }
}
$(function () {
    $("#user_login span").html(user);
    var dpid = getURLParam("dpid");
    controller=getURLParam("controller")
    switchObj={dpid:dpid}
    if(switchObj["dpid"]===""||dpid===undefined){
        alert("Can't get switch infomation,please check ")
        return
    }

    //$("#datatable-table").hide();
    updateSwitchInfo()
    getFlowTables()
    addUpdateMatch()
    addUpdateAction()
    editUpdateAction()
    editUpdateMatch()
})
function updateSwitchInfo(){
    $.get(paths.switchInfo+switchObj.dpid+"?controller="+controller,function(data){
      
        switchObj.hardware=data['hwDesc']
        switchObj.manufacturer=data["mfrDesc"]
        switchObj.openflow=data["openflow"]
        switchObj.software=data['swDesc']
        switchObj.ip=data["inetAddr"].split(":")[0]
        switchObj.port=data["inetAddr"].split(":")[1]
        $("#info-dpid").text(switchObj.dpid)
        $("#info-ip").text(switchObj.ip)
        $("#info-port").text(switchObj.port)
        $("#info-man").text(switchObj.manufacturer)
        $("#info-soft").text(switchObj.software)
        $("#info-hard").text(switchObj.hardware)
        $("#info-proc").text(switchObj.openflow)
    })
}
function getFlowTables() {
    // var ip = $("#control").val().split("|")[0];
    // var port = $("#control").val().split("|")[1];
    if (mockdata == true) {
        var data = [{
            table: 1,
            priority: 1,
            match_fields: "kk=ss,v=111,vv=34,dd=3,tt=1,t2=2,te=4,t3=6",
            actions: "kk=ss,v=111,vv=34,dd=3,tt=1,t2=2,te=4,t3=6",
            otherInfo: "cookie:33,bytes:1111"


        }, {
            table: 1,
            priority: 2,
            match_fields: "kk=ss,v=111,vv=34,dd=3,tt=1,t2=2,te=4,t3=6",
            actions: "kk=ss,v=111,vv=34,dd=3,tt=1,t2=2,te=4,t3=6",
            otherInfo: "cookie:33,bytes:1111"



        }]
        var aaData = [];
        for (var i in data) {
            var checkbox = '<input  onchange="onFlowTableSelect()" class="select-class" type="checkbox">'
            var oper = "<div class='btn-group' role='group'> <a class='btn btn-dark btn-small' onclick='onCickEditButton(" + JSON.stringify(data[i].netport) + ")''><i class='icon-edit'></i> Edit</a>" + "</div>"
            var obj = [
                checkbox,
                data[i].table,
                data[i].priority,
                data[i].match_fields,
                data[i].actions,
                data[i].otherInfo,
                oper
            ];
            aaData.push(obj);
        }
        showTable(aaData);
        $("#datatable-table").show();
        $("#datatable-table").css("width", "100%");
    }
        
    $.get(paths.getFlowTabls+switchObj.dpid+"?controller="+controller, {}, function (data) {
        if (data.retCode == "0" || data.retCode == 0) {
            var fs=_.get(data,"switchflowentry[0].flowEntries")
            _.each(fs,function(v,k){
                v.tableId= +v.tableId
                v.priority= +v.priority
            })
            fs=_.orderBy(fs,["tableId","priority"],['asc', 'desc'])
            flowTables=fs
            var aaData = [];
            _.each(fs,function(v,i){
                var checkbox = '<input  onchange="onFlowTableSelect()" uuid='+v.tableId+' class="select-class" type="checkbox">'
                var oper = "<div class='btn-group' role='group'> <a class='btn btn-dark btn-small' onclick='onCickEditButton(" + JSON.stringify(v) + ")''><i class='icon-edit'></i> Edit</a>" + "</div>"
                var obj = [
                    '',
                    v.tableId,
                    v.priority,
                    getMatchStrAutoBr(v),
                    getActionStrAutoBr(v),
                    getOtherStr(v),
                    oper,
                    v
                ];
                aaData.push(obj);
            })
            showFlowTables(aaData);
        
            $("#datatable-table").show();
            $("#datatable-table").css("width", "100%");
        }else{
            alert(data.retMsg)
        }
    }, "json")
}
function getMatchStr(config){
    var str=""
    _.forOwn(_.get(config,"match"),function(v,k){
        if(v!=undefined||v!=""){
            str+=(" "+k+":"+v)
        }
    })
    return str
}
function getMatchStrAutoBr(config){
    var str=""
    var t=""
    _.forOwn(_.get(config,"match"),function(v,k){
        if(v!=undefined||v!=""){
            t+=(k+" : "+v)
            str+=t
            str+="<br/>"
            t=""
        }
    })
    str+=t;
    return str
}
function getActionStrAutoBr(config){
    var str=""
    _.forIn(getActionObj(config),function(v,k){
        str+=(" "+k+"  <i class='icon-arrow-left'></i>  "+v+"<br/>")
    })
    return str
}
function getActionStr(config){
    var str=""
    _.forIn(getActionObj(config),function(v,k){
        str+=(" "+k+" <i class='icon-arrow-left'></i> "+v)
    })
   
    return str
}
function getOtherStr(config){
    var str=""
    _.forOwn(config,function(v,k){
        if(_.indexOf(["uuid","match","instruction","priority","tableId"],k)== -1){
            if(v!=undefined||v!=""){
                str+=(" "+k+" : "+v+"<br/>")
            }
        }
    })
    return str
}
function setAction(obj,key,v){
    if(key.split(".")[0]=='writeAction'||key.split(".")[0]=="applyAction"){
        if(_.hasIn(flowTableConfig,"instruction."+key)){
            _.set(obj,'instruction.'+key,v)
        }else{
            _.set(obj,'instruction.'+key.split(".")[0]+".setField."+key.split(".")[1],v)
        }
    }else{
        _.set(obj,'instruction.'+key,v)
    }
}
function unsetAction(obj,key){
    if(key.split(".")[0]=='writeAction'||key.split(".")[0]=="applyAction"){
        if(_.hasIn(flowTableConfig,"instruction."+key)){
            _.unset(obj,'instruction.'+key)
        }else{
            _.unset(obj,'instruction.'+key.split(".")[0]+".setField."+key.split(".")[1])
        }
    }else{
        _.unset(obj,'instruction.'+key)
    }
}
function getActionObj(config){
    var obj={}
    _.assign(obj,_.get(config,"instruction"))
    _.assign(obj,_.get(config,"instruction.applyAction"))
    _.assign(obj,_.get(config,"instruction.applyAction.setField"))
    _.assign(obj,_.get(config,"instruction.writeAction"))
    _.assign(obj,_.get(config,"instruction.writeAction.setField"))
    delete obj["applyAction"]
    delete obj["writeAction"]
    delete obj["setField"]
    _.forIn(obj,function(v,k){
        if(v===""||v===undefined){
            delete obj[k]
        }
    })
    return obj
}
function getMatchKeys(){
    return _.keys(_.get(flowTableConfig,"match"))
}
function getActionKeys(){
    var r=[]
    _.forOwn(_.get(flowTableConfig,"instruction.applyAction"),function(v,k){
        if(k!=="setField"){
            r.push(k)
        }
    })
    _.forOwn(_.get(flowTableConfig,"instruction.applyAction.setField"),function(v,k){
        r.push(k)
    })
    return r
}
function selectAll() {
    var isSelect = $("#selectAllCheckbox").prop('checked')
    if (isSelect) {
        $(".flow-tables").find(".select-class").prop("checked", true)
    } else {
        $(".flow-tables").find(".select-class").prop("checked", false)
    }
    onFlowTableSelect()
}

function ifSelected() {
    $("#remove-flow-btn").prop("disabled",_.isEmpty( tableObj.rows('.selected').data()))
}
function onFlowTableDelAllBtn(){
    $('#info-title').text("Confirmation")
    $('#info-body').html('<i class="icon-exclamation-sign"  style="color:#fd1212"></i><span>Delete All Flow Tables  ?</span>')
    $("#info-modal").modal("show")
    $('#info-yes-btn').off("click")
    $('#info-yes-btn').prop("disabled",false)
    $('#info-yes-btn').on("click",function(){
        $('#info-yes-btn').prop("disabled",true)
        _.delay(function(){
            $('#info-body').html('<i class="icon-ok"  style="color:#1fd057"></i><span>Successfull Delete All Flow Tables </span>')
        },500)
    })
}
function onFlowTableDelBtn(){
    if(tableObj.rows('.selected').data().length==tableObj.rows().data().length){
        $('#info-title').text("Confirmation")
        $('#info-body').html('<i class="icon-exclamation-sign"  style="color:#fd1212"></i><span>Delete All Flow Tables  ?</span>')
        $("#info-modal").modal("show")
        $('#info-yes-btn').off("click")
        $('#info-yes-btn').prop("disabled",false)
        $('#info-yes-btn').on("click",function(){
            $('#info-yes-btn').prop("disabled",true)
            $('#info-no-btn').prop("disabled",true)
            $('#info-no-btn').off("click")
            $('#info-no-btn').on("click",function(){
                 window.location.reload()
            })
            $.ajax({
                url: paths.flushTable+"?controller="+controller,
                type: 'DELETE',
                success: function(res) {
                  if(res.retCode==0){
                    $('#info-body').html('<i class="icon-ok"  style="color:#1fd057"></i><span>Successfull Delete All Flow Tables </span>')
                  }else{
                    $('#info-body').html('<i class="icon-exclamation-sign"   style="color:#fd1212"></i><span>Error: '+res.retMsg+'</span>')
                  }
                  $('#info-no-btn').prop("disabled",false)
                },
                error: function(jq,str,msg){
                    $('#info-body').html('<i class="icon-exclamation-sign"   style="color:#fd1212"></i><span>Error: '+msg+'</span>')
                    $('#info-no-btn').prop("disabled",false)
                },
                data:JSON.stringify({
                    "DPID":switchObj.dpid,
                }),
                contentType:"application/json;"                 
            });
        })
    }else{
        $('#info-title').text("Confirmation")
     
        $('#info-yes-btn').off("click")
        $('#info-yes-btn').prop("disabled",false)
        var dellist=[]
        _.each(tableObj.rows('.selected').data(),function(ds){
            var obj=_.last(ds)
            if(!_.isEmpty(obj)){
                dellist.push(obj)
            }
        })
        $('#info-body').empty()
        $('#info-body').html('<i class="icon-exclamation-sign"  style="color:#fd1212"></i><span>Delete Selected Flow Tables  ?</span>')
        $("#info-modal").modal("show")
        $('#info-no-btn').off("click")
        $('#info-yes-btn').on("click",function(){
           $('#info-yes-btn').prop("disabled",true)
           $('#info-no-btn').prop("disabled",true)
           $('#info-no-btn').off("click")
           $('#info-no-btn').on("click",function(){
                window.location.reload()
           })
           var title=$('<i class="icon-exclamation-sign"  style="color:#fd1212"></i><span>Deleting ...  </span>')
           var container=$("<div class='row-container'></div>") 
           $('#info-body').empty().append(title).append(container)
          
           var count=0;
           var delDone=function(){
                if(dellist.length==count){
                    title=$('<i class="icon-ok"  style="color:#1fd057"></i><span>All Done , please check the result </span>')
                    $('#info-body').empty().append(title).append(container)
                    title=$('<i class="icon-ok"  style="color:#1fd057"></i><span>All Done , please check the result </span>')
                    $('#info-no-btn').prop("disabled",false)
                }
           }
            _.each(dellist,function(obj,i){
                $.ajax({
                    url: paths.deleteTable+"?controller="+controller,
                    type: 'DELETE',
                    success: function(res) {
                      if(res.retCode==0){
                        container.append('<div class="item" >uuid: '+obj["uuid"]+'-------------[OK]</div>')
                      }else{
                        container.append('<div class="item" >uuid: '+obj["uuid"]+'-------------[Fail]</div>')
                        container.append('<div class="item" >Error msg:'+res.retMsg+'</div>')
                      }
                      ++count
                      delDone()
                    },
                    error: function(jq,str,msg){
                        container.append('<div class="item" >uuid: '+obj["uuid"]+'-------------[Fail]</div>')
                        container.append('<div class="item" >Error msg: '+msg+'</div>')
                        ++count
                        delDone()
                    },
                    data:JSON.stringify({
                        "DPID":switchObj.dpid,
                        "uuid":obj["uuid"]
                    }),
                    contentType:"application/json;"                 
                });
            })
           
        })
    }

}

function getURLParam(key) {
    var reg = new RegExp("(\\?|&)" + key + "=([^&]*)(&|$)", "i");
    var rs = window.location.search.match(reg);
    if (rs) {
        return unescape(rs[2]);
    }
}

var addConfig 
var addJSONEditor

function onAddFlowTable() {
    var container = document.getElementById('add-config-json');
    $(".add-jsonview2").addClass("collapsed")
    $(".add-jsonview-body2").removeClass("in")
    $(".add-jsonview-body2").css("height", "0px")
    $(".add-jsonview1").removeClass("collapsed")
    $(".add-jsonview-body1").addClass("in")
    $(".add-jsonview-body1").css("height", "auto")
    $("#add-config-json").empty()
    $("#add-config-json").css("height","")
    $("#add-yes-btn").prop("disabled",false)
    $("#add-no-btn").prop("disabled",false)
 
    addConfig  = {
        tableId: 0,
        idleTimeout:200,
        hardTimeout:200
    }
    var options = {
        onChange: _.throttle(function () {
            addConfig = addJSONEditor.get()
            addUpdateConfig(true)
        }, 500)
    };
    addJSONEditor = new JSONEditor(container, options);
    addJSONEditor.set(addConfig)
    addUpdateConfig()
}

function addJSONUpdate() {
    var str = $("#add-json-string").val()
    try {
        if (str != "" && addJSONEditor) {
            var jconfig = JSON.parse(str)
            addConfig = jconfig
            addUpdateConfig()
            $("#add-json-string").val("")

        } else {
            alert("Input your JSON string please")
        }
    } catch (e) {
        alert("Wrong JSON string !")
    }
}

function addJSONGen() {
    $("#add-json-string").val(JSON.stringify(addConfig))
}

function addUpdateMatch() {
    $("#add-match-key").empty()
    var obj=getMatchKeys()
    _.each(obj,function(k){
        $("#add-match-key").append('<option' + ' value='  + k + '  title='  + k + '>' + k + '</option>')
    })
    $("#add-match-key").select2()
}

function addUpdateAction() {
    $("#add-action-key").empty()
    _.each([actionGroup[0]], function (g) {
        $("#add-action-key").append('<optgroup' + ' label=' + g + ' >')
        _.each(getActionKeys(),function(k){
            $("#add-action-key").append('<option' + ' value=' + g + '.' + k + '  title=' + g + '.' + k + '>' + k + '</option>')
        })
        $("#add-action-key").append('</optgroup>')

    })
    _.forOwn(_.get(flowTableConfig,"instruction"),function(v,k){
        if(k!="applyAction"&&k!="writeAction"){
            $("#add-action-key").append('<option' + ' value='  + k + '  title='  + k + '>' + k + '</option>')
        }
    })
    $("#add-action-key").select2({
        templateSelection: function (s) {
            return s.id
        }
    })
}

function addUpdateConfig(notUpdateEditor) {
    if (addJSONEditor && !notUpdateEditor) {
        addJSONEditor.set(addConfig)
    }
    var container = $(".add-jsonview-body1")
    container.find(".priority").val(addConfig.priority)
    container.find(".tableId").val(addConfig.tableId)
    container.find(".hardTimeout").val(addConfig.hardTimeout)
    container.find(".idleTimeout").val(addConfig.idleTimeout)
    container.find(".match-result").empty()
    container.find(".match-result").append(getMatchStr(addConfig))
    
    container.find(".action-result").empty()
    container.find(".action-result").append(getActionStr(addConfig))
   
    addActionKeyChange()
    addMatchKeyChange()
}

function addInfoChange() {
    _.throttle(function () {
        var container = $(".add-jsonview-body1")
        if(!_.isEmpty(container.find(".priority").val())){
            addConfig.priority = container.find(".priority").val()
        }else{
            delete addConfig.priority
        }
        if(!_.isEmpty(container.find(".hardTimeout").val())){
            addConfig.hardTimeout=container.find(".hardTimeout").val()
        }else{
            delete addConfig.hardTimeout
        }
        if(!_.isEmpty(container.find(".idleTimeout").val())){
            addConfig.idleTimeout=container.find(".idleTimeout").val()
        }else{
            delete addConfig.idleTimeout
        }
        if(!_.isEmpty(container.find(".tableId").val())){
            addConfig.tableId=container.find(".tableId").val()
        }else{
            delete addConfig.tableId
        }
        addUpdateConfig()
    }, 500)()
}


function addMatchValueChange() {
    _.throttle(function () {
        var container = $(".add-jsonview-body1")
        var value=_.trim(container.find(".match-value").val())
        if(_.isEmpty(value)){
            _.unset(addConfig,"match."+container.find("#add-match-key").val())
        }else{
            _.set(addConfig,"match."+container.find("#add-match-key").val(),container.find(".match-value").val())
        }
        addUpdateConfig()
    }, 500)()
}

function addActionValueChange() {
    _.throttle(function () {
        var container = $(".add-jsonview-body1")
        var value=_.trim(container.find(".action-value").val())
        if(_.isEmpty(value)){
            unsetAction(addConfig,container.find("#add-action-key").val())
        }else{
            setAction(addConfig,container.find("#add-action-key").val(),container.find(".action-value").val())
        }
        addUpdateConfig()
    }, 500)()
}

function addMatchKeyChange() {
    var container = $(".add-jsonview-body1")
    var key = container.find("#add-match-key").val()
    var value=_.get(addConfig, "match." + key)
    container.find(".match-value").val(value)
    var lastKey=_.last(key.split("."))
    if(checkRules[lastKey]){
        var checker=checkRules[lastKey]
        container.find(".match-info").html(checker.des)
        if(checker.isMatch(value)||_.isEmpty(value)){
            container.find(".match-value").removeClass("warning")
            container.find(".match-info").removeClass("info-warning")
        }else{
            container.find(".match-value").addClass("warning")
            container.find(".match-info").addClass("info-warning")
        }
    }else{
        container.find(".match-value").removeClass("warning")
        container.find(".match-info").removeClass("info-warning")
        container.find(".match-info").html("")
    }
}

function addActionKeyChange() {
    var container = $(".add-jsonview-body1")
    var key = container.find("#add-action-key").val()
    var value=_.get(addConfig, "instruction." + key)||_.get(addConfig, "instruction." + key.split(".")[0]+".setField."+key.split(".")[1])
    container.find(".action-value").val(value)
    var lastKey=_.last(key.split("."))
    if(checkRules[lastKey]){
        var checker=checkRules[lastKey]
        container.find(".action-info").html(checker.des)
        if(checker.isMatch(value)||_.isEmpty(value)){
            container.find(".action-value").removeClass("warning")
            container.find(".action-info").removeClass("info-warning")
        }else{
            container.find(".action-value").addClass("warning")
            container.find(".action-info").addClass("info-warning")
        }
    }else{
        container.find(".action-value").removeClass("warning")
        container.find(".action-info").removeClass("info-warning")
        container.find(".action-info").html("")
    }
}
function addOnSaveBtn(){
   $("#add-yes-btn").prop("disabled",true)
   $("#add-no-btn").prop("disabled",true)

   $("#add-no-btn").off("click").on("click",function(){
        window.location.reload()
   })
    $.ajax({
        url: paths.addTable+"?controller="+controller,
        type: 'POST',
        success: function(res) {
          if(res.retCode==0){
            window.location.reload()
           
          }else{
            $("#add-no-btn").prop("disabled",false)
            alert("Error; "+res.retMsg)
          }
        },
        error: function(jq,str,msg){
           $("#add-no-btn").prop("disabled",false)
           alert("Error; "+msg)
        },
        data:JSON.stringify(_.set(_.cloneDeep(addConfig),"DPID",switchObj.dpid)),
        contentType:"application/json;"                 
    });
}

var editConfig
var editJSONEditor
var editUUID
var editBeforeConfig

function onCickEditButton(obj) {
    $("#editModal").modal("show")
    var container = document.getElementById('edit-config-json');
    $(".edit-jsonview2").addClass("collapsed")
    $(".edit-jsonview-body2").removeClass("in")
    $(".edit-jsonview-body2").css("height", "0px")
    $(".edit-jsonview1").removeClass("collapsed")
    $(".edit-jsonview-body1").addClass("in")
    $(".edit-jsonview-body1").css("height", "auto")
    $("#edit-config-json").empty()
    $("#edit-config-json").css("height","")
    $("#editModal").modal({
        backdrop:"auto",
        keyboard:true
    })
    editConfig = obj
    editBeforeConfig=_.cloneDeep(obj)
    editUUID=obj["uuid"]
    delete editConfig["uuid"]
    delete editConfig["createTime"]
    var options = {
        onChange: _.throttle(function () {
            editConfig = editJSONEditor.get()
            editUpdateConfig(true)
        }, 500)
    };
    editJSONEditor = new JSONEditor(container, options);
    editJSONEditor.set(editConfig)
   // editUpdateMatch()
   // editUpdateAction()
    editUpdateConfig()

}
function onClickEditSaveBtn(){
    var beforeConfigObj=_.cloneDeep(editBeforeConfig)
    var afterConfigObj=_.cloneDeep(editConfig)
    

    $("#edit-yes-btn").prop("disabled",true)
    $("#edit-no-btn").prop("disabled",true)
    $("#eidt-no-btn").off("click").on("click",function(){
         window.location.reload()
    })
  
    if(beforeConfigObj["tableId"]==afterConfigObj["tableId"]&&beforeConfigObj["priority"]==afterConfigObj['priority']&&_.isEqual(beforeConfigObj["match"],afterConfigObj["match"])){
        console.log(beforeConfigObj,afterConfigObj)
        var editObj=_.cloneDeep(editConfig)
        editObj["DPID"]=switchObj.dpid
        editObj["uuid"]=editUUID
        
        $.ajax({
            url: paths.editTable+"?controller="+controller,
            type: 'PUT',
            success: function(res) {
              if(res.retCode==0){
                window.location.reload()
              
              }else{
                $("#edit-no-btn").prop("disabled",false)
                alert("Error; "+res.retMsg)
              }
            },
            error: function(jq,str,msg){
               $("#edit-no-btn").prop("disabled",false)
               alert("Error; "+msg)
            },
            data:JSON.stringify(editObj),
            contentType:"application/json;"                 
        });
       
    }else{
        
         $.ajax({
            url: paths.deleteTable+"?controller="+controller,
            type: 'DELETE',
            success: function(res) {
                if(res.retCode==0){
                    $.ajax({
                        url: paths.addTable+"?controller="+controller,
                        type: 'POST',
                        success: function(res) {
                          if(res.retCode==0){
                            window.location.reload()
                           
                          }else{
                            $("#edit-no-btn").prop("disabled",false)
                            alert("Error; "+res.retMsg)
                          }
                        },
                        error: function(jq,str,msg){
                            $("#edit-no-btn").prop("disabled",false)
                            alert("Error; "+res.retMsg)
                        },
                        data:JSON.stringify(_.set(_.cloneDeep(editConfig),"DPID",switchObj.dpid)),
                        contentType:"application/json;"                 
                    });
                    
                }else{
                     $("#edit-no-btn").prop("disabled",false)
                     alert("Error; "+res.retMsg)
                }
            },
            error: function(jq,str,msg){
                $("#edit-no-btn").prop("disabled",false)
                alert("Error; "+msg)
            },
            data:JSON.stringify({
                "DPID":switchObj.dpid,
                "uuid":editUUID
            }),
            contentType:"application/json;"                 
        });
    }
   
}
function editUpdateMatch() {
    $("#edit-match-key").empty()
    _.each(getMatchKeys(),function(k){
        $("#edit-match-key").append('<option' + ' value='  + k + '  title='  + k + '>' + k + '</option>')
    })

    $("#edit-match-key").select2()
}

function editUpdateAction() {
    $("#edit-action-key").empty()
    _.each(actionGroup, function (g) {
        $("#edit-action-key").append('<optgroup' + ' label=' + g + ' >')
        _.each(getActionKeys(),function(k){
            $("#edit-action-key").append('<option' + ' value=' + g + '.' + k + '  title=' + g + '.' + k + '>' + k + '</option>')
        })
        $("#edit-action-key").append('</optgroup>')

    })
    _.forOwn(_.get(flowTableConfig,"instruction"),function(v,k){
        if(k!="applyAction"&&k!="writeAction"){
            $("#edit-action-key").append('<option' + ' value='  + k + '  title='  + k + '>' + k + '</option>')
        }
    })
    $("#edit-action-key").select2({
        templateSelection: function (s) {
            return s.id
        }
    })
}

function editUpdateConfig(notUpdateEditor) {
    if (editJSONEditor && !notUpdateEditor) {
        editJSONEditor.set(editConfig)
        editJSONEditor.expandAll()
    }
    var container = $(".edit-jsonview-body1")
    container.find(".priority").val(editConfig.priority)
    container.find(".tableId").val(editConfig.tableId)
    container.find(".hardTimeout").val(editConfig.hardTimeout)
    container.find(".idleTimeout").val(editConfig.idleTimeout)
    container.find(".match-result").empty().append(getMatchStr(editConfig))
   
   
    container.find(".action-result").empty().append(getActionStr(editConfig))
    
    var beforeConfigObj=_.assign({},editBeforeConfig)
    var afterConfigObj=_.assign({},editConfig)
    delete beforeConfigObj["uuid"]
    $("#edit-yes-btn").prop("disabled",_.isEqual(afterConfigObj,beforeConfigObj))
    editActionKeyChange()
    editMatchKeyChange()
}

function editInfoChange() {
    _.throttle(function () {
        var container = $(".edit-jsonview-body1")
        if(container.find(".priority").val()!=""){
            editConfig.priority = container.find(".priority").val()
        }else{
            delete editConfig.priority
        }
        if(container.find(".hardTimeout").val()!=""){
            editConfig.hardTimeout=container.find(".hardTimeout").val()
        }else{
            delete editConfig.hardTimeout
        }
        if(container.find(".idleTimeout").val()!=""){
            editConfig.idleTimeout=container.find(".idleTimeout").val()
        }else{
            delete editConfig.idleTimeout
        }
        if(container.find(".tableId").val()!=""){
            editConfig.tableId=container.find(".tableId").val()
        }else{
            delete editConfig.tableId
        }
        editUpdateConfig()
    }, 500)()
}


function editMatchValueChange() {
    _.throttle(function () {
        var container = $(".edit-jsonview-body1")
        
        var value=_.trim(container.find(".match-value").val())
        if(_.isEmpty(value)){
            _.unset(editConfig,"match."+container.find("#edit-match-key").val())
        }else{
            _.set(editConfig,"match."+container.find("#edit-match-key").val(),container.find(".match-value").val())
        }
        editUpdateConfig()
    }, 500)()
}

function editActionValueChange() {
    _.throttle(function () {
        var container = $(".edit-jsonview-body1")
        var value=_.trim(container.find(".action-value").val())
        if(_.isEmpty(value)){
            unsetAction(editConfig,container.find("#edit-action-key").val())
        }else{
            setAction(editConfig,container.find("#edit-action-key").val(),container.find(".action-value").val())
        }
        editUpdateConfig()
    }, 500)()
}

function editMatchKeyChange() {
    var container = $(".edit-jsonview-body1")
    var key = container.find("#edit-match-key").val()
    var value=_.get(editConfig, "match." + key)
    container.find(".match-value").val(value)
    var lastKey=_.last(key.split("."))
  
    if(checkRules[lastKey]){
        var checker=checkRules[lastKey]
        container.find(".match-info").html(checker.des)
        if(checker.isMatch(value)||_.isEmpty(value)){
            container.find(".match-value").removeClass("warning")
            container.find(".match-info").removeClass("info-warning")
        }else{
            container.find(".match-value").addClass("warning")
            container.find(".match-info").addClass("info-warning")
        }
    }else{
        container.find(".match-value").removeClass("warning")
        container.find(".match-info").removeClass("info-warning")
        container.find(".match-info").html("")
    }
}

function editActionKeyChange() {
    var container = $(".edit-jsonview-body1")
    var key = container.find("#edit-action-key").val()
    var value=_.get(editConfig, "instruction." + key)||_.get(editConfig, "instruction." + key.split(".")[0]+".setField."+key.split(".")[1])
    container.find(".action-value").val(value)
    var lastKey=_.last(key.split("."))
    if(checkRules[lastKey]){
        var checker=checkRules[lastKey]
        container.find(".action-info").html(checker.des)
        if(checker.isMatch(value)||_.isEmpty(value)){
            container.find(".action-value").removeClass("warning")
            container.find(".action-info").removeClass("info-warning")
        }else{
            container.find(".action-value").addClass("warning")
            container.find(".action-info").addClass("info-warning")
        }
    }else{
        container.find(".action-value").removeClass("warning")
        container.find(".action-info").removeClass("info-warning")
        container.find(".action-info").html("")
    }
}

function editJSONUpdate() {
    var str = $("#edit-json-string").val()
    try {
        if (str != "" && editJSONEditor) {
            var jconfig = JSON.parse(str)
            editConfig = jconfig
            editUpdateConfig()
            $("#edit-json-string").val("")

        } else {
            alert("Input your JSON string please")
        }
    } catch (e) {
        alert("Wrong JSON string !")
    }
}

function editJSONGen() {
    $("#edit-json-string").val(JSON.stringify(editConfig))
}
$('#collapseOne').on('hide.bs.collapse', function (e) {
    $("#add-config-json").css("height","400px")
})
$('#collapseOne').on('show.bs.collapse', function (e) {
    $("#add-config-json").css("height","")
})
$('#collapseOne4').on('hide.bs.collapse', function (e) {
    $("#edit-config-json").css("height","400px")
})
$('#collapseOne4').on('show.bs.collapse', function (e) {
    $("#edit-config-json").css("height","")
})