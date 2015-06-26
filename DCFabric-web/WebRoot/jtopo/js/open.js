var prefix = "../jtopo/";
(function(exports){

    function randomDatas(){
        var rs = '';
        var p = 4; // 4台物理机
        var swCounter = 0;
        for(var i=0; i<5; i++){ // 5个子网
            var pIndex = Math.floor(Math.random() * 4);

            for(var j=0; j<4; j++){ // 每个子网4个核心交换机
                var id = 'core-' + i + '-' + j;
                var name = id;
                var upId = Math.floor(Math.random() * p);
                var downId = Math.floor(Math.random() * p);
                while(downId == upId){
                    downId = Math.floor(Math.random()* p);
                }

                if(pIndex == j){ // 和物理连接
                var str = '\n{"id": "' + id+ '", "name": "'+name+'", "core":"true", "upNode":["'+upId+'"], '+
                    '"downNode":["'+downId+'"], "mininet":"'+i+'"},';
                rs += str;
                }else{
                    var str = '\n{"id": "' + id+ '", "name": "'+name+'", "core":"true", "upNode":[], '+
                        '"downNode":[], "mininet":"'+i+'"},';
                    rs += str;
                }

                var s = 75/4; // 每个子网最多有75个非交换机

                for(var m=0; m <s; m++){
                    id = 'sw-' + (swCounter++);
                    name = id;
                    upId = 'core-' + i + '-' + Math.floor(Math.random() * 4);
                    downId = 'core-' + i + '-' + Math.floor(Math.random() * 4);
                    while(downId == upId){
                        downId = 'core-' + i + '-' + Math.floor(Math.random() * 4);
                    }
                    str = '\n{"id": "' + id+ '", "name": "'+name+'", "core":"false", "upNode":["'+upId+'"], '+
                        '"downNode":["'+downId+'"], "mininet":"'+i+'"},';
                    rs += str;
                }
            }
        }
        console.log(rs);
    }

    var host_counter = 0;
    function randomHost(){
        var id = 'H_' + (host_counter++);
        return {
            id : 'H_' + id,
            name:  'H_' + id
        };
    }

    function isPhysical(e){
        if(e["isPhysical"] == null){
            return e["mininet"] == null || e["mininet"] == "";
        }else{
            return e["isPhysical"] == "1";
        }
    }

    function isCoreSwitch(e){
        return (e["core"] == true || e["core"] == "true") && !isPhysical(e);
    }

    function notCoreSwitch(e){
        if(e.type == 'host') return false;
        return ( e["core"] == null || e["core"] == false || e["core"] == "false") && !isPhysical(e);
    }

    // 是否属于某一个子网
    function isBelongsMininet(e){
        return e['mininet'] != null && e['mininet'] != "";
    }

    // 获取子网信息
    function getMininetInfo(datas){
        var mininetCount = 0;  // 子网总个数
        var map = {};
        var names = [];
        datas.forEach(function(e){
            // 计算子网个数
            if(isBelongsMininet(e)){
                var name = e["mininet"];
                if(map[name] == null){
                    map[name] = [];
                    names.push(name);
                    mininetCount++;
                }
                map[name].push(e);
            }
        });
        var rs = {
            names: names,
            map: map,
            count: mininetCount
        }
        return rs;
    }


    function switchToNode(obj){
        var node = new JTopo.Node(obj.name);
        if(isPhysical(obj)){
            node.setImage(prefix+'img/wulijiaohuanji.png', false);
            node.setSize(54,54);
        }else if(isCoreSwitch(obj)){
            node.setImage(prefix+'img/hexinjiaohuanji.png', false);
            node.setSize(48,48);
        }else{
            node.setImage(prefix+'img/jiaohuanji.png', false);
            node.setSize(48,48);
        }
        //node.textPosition = 'Middle_Center';
        node.id = obj.id;
        node.type = 'switch';
        node.data = obj;
        node.fontColor = "0,0,0"
        node.font = "14px Consolas"
        if(obj.x){
            try{
                node.x = parseInt(obj.x);
            }catch(e){
            }
        }
        if(obj.y){
            try{
                node.y = parseInt(obj.y);
            }catch(e){
            }
        }
        return node;
    }

    function hostToNode(obj){
        var node = new JTopo.Node(obj.name);
        node.setImage(prefix+'img/host.png', true);
        node.id = obj.id;
        node.type = 'host';
        node.data = obj;
        node.fontColor = "0,0,0"
        node.font = "14px Consolas"
        return node;
    }

    function getNodeDiameter(node){
        var diameter = 0;
        if(node.diameter){
            diameter = node.diameter;
        }else if(node.radius){
            diameter = node.radius * 2;
        }else{
            diameter = Math.sqrt(2 * node.width * node.height);
        }
        return diameter
    }

    /**
     * 获取一些节点的中心点
     */
    function getNodesCenter(nodes){
        var x = 0;
        var y = 0;
        nodes.forEach(function(e){
            x += e.cx;
            y += e.cy;
        });
        var rs = {x: x/nodes.length, y: y/nodes.length};
        return rs;
    }

    // 批量移动节点
    function moveNodes(nodes, dx, dy){
        nodes.forEach(function(node){
            node.x += dx;
            node.y += dy;
        });
    }

    /**
     * 圆形布局计算
     *@param cx 圆心 x
     *@param cy 圆心 y
     *@param nodes 要布局的节点
     *@param nodeDiameter 节点的直径
     *@param hScale 水平缩放系数
     *@param vScale 垂直缩放系数
     *@param beginAngle 开始角度
     *@param endAngle 结束角度
     *@param minRradius 最小半径
     *@return {Array} 圆的半径[水平、垂直]
     */
    function getCircleLayout(nodes, opt){
        // 自动计算时，至少需要两个节点来计算出大圆半径，没有非核心的时候，模拟出来几个（仅仅布局计算坐标用）
        if(nodes.length == 0 ){
            nodes = [{radius: 64}, {radius: 64}];
        }else if(nodes.length == 1){
            var cloneNode = {};
            cloneNode.radius = nodes[0].radius;
            cloneNode.width = nodes[0].width;
            cloneNode.height = nodes[0].height;
            cloneNode.diameter = nodes[0].diameter;
            nodes.push(cloneNode);
        }

        if(opt == null) opt = {};
        var cx = opt.cx;
        var cy = opt.cy;
        var minRadius = opt.minRadius;
        var nodeDiameter = opt.nodeDiameter;
        var hScale = opt.hScale || 1;
        var vScale = opt.vScale || 1;
        var beginAngle = opt.beginAngle || 0;
        var endAngle = opt.endAngle || (2 * Math.PI);
        var totalAngle = endAngle - beginAngle;

        // 中心点
        if(cx == null || cy == null){
            var center = getNodesCenter(nodes);
            cx = center.x;
            cy = center.y;
        }

        // 周长和每个节点的直径
        var diameters = nodes.map(getNodeDiameter);
        var perimeter = diameters.reduce(function(a, b){return a + b}, 0);

        // 每个节点的角度
        var angles = [];
        nodes.forEach(function(node, index){
            var rate = diameters[index] / perimeter;
            angles.push(totalAngle/2 * rate);
        });

        var count = nodes.length;
        var	angle = angles[0];
        var diameter = diameters[0];
        var circleRadius = (diameter/2) / Math.sin(angle);
        if(minRadius != null && minRadius > circleRadius){
            circleRadius = minRadius;
        }
        if(opt.dxRadius){
            circleRadius += opt.dxRadius;
        }
        var circleRadiusA = circleRadius * hScale;
        var circleRadiusB = circleRadius * vScale;

        var da = beginAngle;
        var locations = [];
        var nodeAngles = [];// 记录每个节点的角度
        nodes.forEach(function(node, index){
            da += index == 0 ? angles[index]: angles[index-1] + angles[index];
            nodeAngles.push(da);
            var x = cx + Math.cos(da) * circleRadiusA;
            var y = cy + Math.sin(da) * circleRadiusB;
            locations.push({cx: x, cy: y});
        });

        return {cx: cx, cy:cy,
            radius: circleRadiusA, locations: locations, angles: nodeAngles,
            radiusA: circleRadiusA, radiusB: circleRadiusB
        };
    }

    function layoutNodes(nodes, locations){
        nodes.forEach(function(node, index){
            node.cx = locations[index].cx;
            node.cy = locations[index].cy;
        });
    }

    function treeLayoutNodes(cx, startY, nodes, width, gap){
        var count = nodes.length;
        gap = gap || 20;
        var nodeWidth = width || nodes[0].width;
        var startX = cx - (nodeWidth + gap) * count /2;
        nodes.forEach(function(node, index){
            node.x = startX + (index * (gap + nodeWidth));
            node.y = startY;
        });
    }

    function Container(){
        var container = new JTopo.Container();
        return container;
    }

    // 连线
    function linkNodes(nodes, scene, nodeMap, linkMap){
        function link(nodeA, nodeZ, portA, portZ){
            var k1 = nodeA.id + nodeZ.id;
            var k2 = nodeZ.id + nodeA.id;
            if(linkMap[k1] == null || linkMap[k2] == null){
                var link = new JTopo.Link(nodeA, nodeZ);
                link.lineWidth = 1;
                link.alpha = 0.4;
                link.portA = portA;
                link.portZ = portZ;
                linkMap[k1] = link;
                linkMap[k2] = link;
                scene.add(link);
            }
        }

        nodes.forEach(function(e){
            var node = nodeMap[e.id];
            if(node == null) return;
            var upNodes = e['upNode'];
            if(upNodes && upNodes.length > 0) {
                for(var i=0; i<upNodes.length; i++){
                    var upObj = upNodes[i];
                    if(typeof upObj == 'string'){
                        upObj = {id: upObj, portA:'', portZ:''};
                    }
                    var upId = upObj.id;
                    var upNode = nodeMap[upId];
                    if (upNode) {
                        link(upNode, node, upObj.portA, upObj.portZ);
                    }
                }
            }

            var downNodes = e['downNode'];
            if(downNodes && downNodes.length > 0) {
                for(var i=0; i<downNodes.length; i++) {
                    var downObj = downNodes[i];
                    if(typeof downObj == 'string'){
                        downObj = {id: downObj, portA:'', portZ:''};
                    }
                    var downId = downObj.id;
                    var downNode = nodeMap[downId];
                    if (downNode) {
                        link(node, downNode, downObj.portA, downObj.portZ);
                    }
                }
            }
        });
    }

    // 树形布局
    function initTree(data, canvas){
        this.treeData = data;
        this.treeStage = new JTopo.Stage(canvas);
        this.stage = this.treeStage;
        //this.treeStage.wheelZoom = 1.2;
        this.treeStage.mousewheel(function(e){
            if(e.wheelDelta){
                if(e.wheelDelta > 0){
                    Topo.zoomNodes(1.1);
                }else{
                    Topo.zoomNodes(0.9);
                }
            }
            if(e.detail){
                if(e.detail < 0){
                    Topo.zoomNodes(1.1);
                }else{
                    Topo.zoomNodes(0.9);
                }
            }
        });

        this.treeStage.eagleEye.visible = true;
        var nodeMap = this.treeNodeMap;
        var linkMap = {};

        var scene = new JTopo.Scene(this.treeStage);
        //scene.background = './img/bg.jpg';
        var cx = canvas.width/2;
        var cy = canvas.height/2;

        function addToScene(obj){
            scene.add(obj);
            nodeMap[obj.id] = obj;
        }

        // 物理节点
        var pyhsicals = data.nodes.filter(isPhysical);
        var pyhsicalNodes = pyhsicals.map(switchToNode);
        pyhsicalNodes.forEach(addToScene);
        console.log('物理节点个数' + pyhsicalNodes.length);

        // 核心交换机
        var coreSwitchs = data.nodes.filter(isCoreSwitch);
        var coreSwitchNodes = coreSwitchs.map(switchToNode);
        coreSwitchNodes.forEach(addToScene);

        //非核心
        var switchs = data.nodes.filter(notCoreSwitch);
        var switchNodes = switchs.map(switchToNode);
        switchNodes.forEach(addToScene);

        // 布局
        function layoutAll(){
            // 如果不需要自动布局
            if(data["need-layout"] != true && data["need-layout"] != "true"){
                return;
            }

            // 物理
            treeLayoutNodes(cx, 50, pyhsicalNodes, 256, 20);
            // 核心
            treeLayoutNodes(cx, 200, coreSwitchNodes, 256, 20);
            // 非核心
            if(switchNodes.length >= coreSwitchNodes.length *2){
                treeLayoutNodes(cx, 400, switchNodes.filter(function(e,i){return i% 2 == 0;}), null, 20);
                treeLayoutNodes(cx, 500, switchNodes.filter(function(e,i){return i% 2 != 0;}), null, 20);
            }else if(switchNodes.length >= coreSwitchNodes.length * 3){
                treeLayoutNodes(cx, 400, switchNodes.filter(function(e,i){return i% 2 == 0;}), null, 20);
                treeLayoutNodes(cx, 500, switchNodes.filter(function(e,i){return i% 3 == 0;}), null, 20);
                treeLayoutNodes(cx, 600, switchNodes.filter(function(e,i){return i% 2 != 0 && i%3 !=0;}), null, 20);
            }else{
                treeLayoutNodes(cx, 400, switchNodes, 256, 20);
            }
        }
        layoutAll();
        linkNodes(data.nodes, scene, nodeMap, linkMap);
        // 双击动态展开下面的主机
        scene.dbclick(function(event){
            if(event.target == null) return;
            var node = event.target;
            var coreSwitch = node.data;
            if(! isPhysical(coreSwitch)){
                if(node.hostLinks){
                    node.hostLinks.forEach(function(n){
                        scene.remove(n);
                    });
                    node.hostNodes.forEach(function(n){
                        scene.remove(n);
                    });
                    scene.remove(node.hostContainer);
                    node.hostContainer = null;
                    node.hostNodes = null;
                    node.hostLinks = null;
                }else{
                    Topo.loadHostDatas(coreSwitch.id).done(function(response){
                        if(!response || !response.nodes) return;
                        var hosts = response.nodes;
                        if(hosts.length == 0) return;
                        node.hostNodes = hosts.map(hostToNode);

                        node.hostContainer = Container(' ');
                        node.hostContainer.zIndex = 3;
                        node.hostContainer.fillColor = '191,191,191';

                        node.hostNodes.forEach(function(n){
                            node.hostContainer.add(n);
                            n.zIndex = node.hostContainer.zIndex + 1;
                            addToScene(n);
                        });
                        scene.add(node.hostContainer);
                        node.hostContainer.x = event.x;
                        node.hostContainer.y = event.y;

                        node.hostLinks = [];
                        node.hostNodes.forEach(function(hostNode){
                            var link = new JTopo.Link(node, hostNode);
                            link.alpha = 0.4;
                            link.portZ = node.portA;
                            node.hostLinks.push(link);
                            scene.add(link);
                        });

                        if(isCoreSwitch(coreSwitch)){
                            treeLayoutNodes(node.cx, node.cy + 350, node.hostNodes, 64, 10);
                        }else{
                            treeLayoutNodes(node.cx, node.cy + 200, node.hostNodes, 64, 10);
                        }
                    });
                }
            }
        });

        // 定时获取动态增加、删除的节点
        function dynamicAddDelete(){
            try{
                Topo.loadDynamicDatas().done(function(response){
                    if(response.add && response.add.length > 0){
                        // 过滤掉那些已经存在的
                        var datas = response.add.filter(function(e){ return nodeMap[e.id] == null; });
                        if(datas && datas.length > 0){
                            var nodes = datas.map(switchToNode);
                            nodes.forEach(addToScene);
                            nodes.forEach(function(node){
                                if(isPhysical(node.data)){
                                    pyhsicalNodes.push(node);
                                }else if(isCoreSwitch(node.data)){
                                    coreSwitchNodes.push(node);
                                }else if(notCoreSwitch(node.data)){
                                    switchNodes.push(node);
                                }
                            });
                            layoutAll();
                            linkNodes(datas, scene, nodeMap, linkMap);
                            // 闪烁
                            Topo.highlightNodes(nodes, 10);
                        }
                    }
                    if(response["delete"] && response["delete"].length > 0){
                        var ids = response["delete"];
                        Topo.remove(Topo.treeStage, ids);
                    }
                });
            }catch(e){
                console.log('查询动态节点出错:' + e);
            }
            //setTimeout(dynamicAddDelete, 3000);
        }
        //setTimeout(dynamicAddDelete, 3000);
        //this.treeStage.centerAndZoom();
    }

    // 圆形布局
    function initCircle(data, canvas){
        this.circleData = data;
        this.circleStage = new JTopo.Stage(canvas);
        //this.circleStage.wheelZoom = 1.2;
        this.circleStage.mousewheel(function(e){
            if(e.wheelDelta){
                if(e.wheelDelta > 0){
                    Topo.zoomNodes(1.1);
                }else{
                    Topo.zoomNodes(0.9);
                }
            }
            if(e.detail){
                if(e.detail < 0){
                    Topo.zoomNodes(1.1);
                }else{
                    Topo.zoomNodes(0.9);
                }
            }
        });

        this.circleStage.eagleEye.visible = true;
        var scene = new JTopo.Scene(this.circleStage);
        //scene.background = './img/bg.jpg';
        var nodeMap = this.circleNodeMap;
        var linkMap = {};

        var cx = canvas.width/2;
        var cy = canvas.height/2;

        // 子网信息
        var mininetInfo = getMininetInfo(data.nodes);
        var mininetMap = mininetInfo.map; //map
        var mininetNames = mininetInfo.names; //名称列表
        var mininetNumber =  mininetNames.length; // 子网数量

        function addToScene(node){
            scene.add(node);
            nodeMap[node.id] = node;
        }

        // 物理节点
        var pyhsicals = data.nodes.filter(isPhysical);
        var pyhsicalNodes = pyhsicals.map(switchToNode);
        pyhsicalNodes.forEach(addToScene);
        console.log('圆形-物理数量：' + pyhsicals.length);

        // 核心交换机
        var coreSwitchs = data.nodes.filter(isCoreSwitch);
        var coreSwitchNodes = coreSwitchs.map(switchToNode);
        coreSwitchNodes.forEach(addToScene);
        //coreSwitchNodes.forEach(function(e){e.text = "核心-" + e.text;});
        console.log('圆形-核心交换机：' + coreSwitchs.length);

        //非核心
        var switchs = data.nodes.filter(notCoreSwitch);
        var switchNodes = switchs.map(switchToNode);
        switchNodes.forEach(addToScene);
        //switchNodes.forEach(function(e){e.text = "交换-" + e.text;});
        console.log('圆形-非核心交换机：' + switchs.length);

        function layoutAll(){
            // 如果不需要自动布局
            if(data["need-layout"] != true && data["need-layout"] != "true"){
                return;
            }

            // 布局物理节点
            var phyLayout = getCircleLayout(pyhsicalNodes, {cx: cx, cy: cy});
            layoutNodes(pyhsicalNodes, phyLayout.locations);

            // 布局核心交换机
            // 模拟 先计算每个子网的半径
            var mininetRadiusNodes = [];
            for(var i=0; i<mininetNumber; i++){
                var name = mininetNames[i];
                var subNodes = switchNodes.filter(function(node){ return node.data.mininet == ''+name; });
                var layout = getCircleLayout(subNodes, {cx: cx, cy: cy, minRadius: 250});
                var virtualNode = {
                    radius: layout.radius + 32
                };
                mininetRadiusNodes.push(virtualNode);
            }

            var phyLayoutRadius = 128;
            if(phyLayout== null || isNaN(phyLayout.radius) || phyLayout.radius < 128 ){
                phyLayoutRadius = 128;
            }else{
                phyLayoutRadius = phyLayout.radius + 128;
            }
            var virtualLayout = getCircleLayout(mininetRadiusNodes, {cx: cx, cy: cy, dxRadius: phyLayoutRadius});
            var swCenters = virtualLayout.locations; // 核心交换机的中心列表

            var angle = 2 * Math.PI / mininetNumber;
            for(var i=0; i<mininetNumber; i++){
                var name = mininetNames[i];
                var subNodes = coreSwitchNodes.filter(function(node){ return node.data.mininet == ''+name; });
                console.log('子网:'+name + ' 节点数量:' + subNodes.length);
                var x = swCenters[i].cx;
                var y = swCenters[i].cy;
                var layout = getCircleLayout(subNodes, {cx: x, cy: y});
                layoutNodes(subNodes, layout.locations);
            }

            // 布局非核心交换机
            for(var i=0; i<mininetNumber; i++){
                var name = mininetNames[i];
                var subNodes = switchNodes.filter(function(node){ return node.data.mininet == ''+name; });
                console.log('子网：'+name + '非核心节点数量:' + subNodes.length);
                var x = swCenters[i].cx;
                var y = swCenters[i].cy;
                var layout = getCircleLayout(subNodes, {cx: x, cy: y, minRadius: 150});
                var angles = layout.angles;
                subNodes.forEach(function(n, index){
                    n.angle = angles[index];
                });
                layoutNodes(subNodes, layout.locations);
            }
        }

        setTimeout(function(){
            layoutAll();
            Topo.circleStage.eagleEye.update();
        }, 500);

        linkNodes(data.nodes, scene, nodeMap, linkMap);

        // 双击交换机展开下面的主机
        scene.dbclick(function(event){
            if(event.target == null) return;
            var node = event.target;
            var coreSwitch = node.data;
            if(! isPhysical(coreSwitch)){
                if(node.hostLinks){
                    node.hostLinks.forEach(function(n){
                        scene.remove(n);
                    });
                    node.hostNodes.forEach(function(n){
                        scene.remove(n);
                    });
                    scene.remove(node.hostContainer);
                    node.hostContainer = null;
                    node.hostNodes = null;
                    node.hostLinks = null;
                }else{
                    Topo.loadHostDatas(coreSwitch.id).done(function(response){
                        if(!response || !response.nodes) return;
                        var hosts = response.nodes;
                        if(hosts.length == 0) return;
                        node.hostNodes = hosts.map(hostToNode);

                        node.hostContainer = Container(coreSwitch.id);
                        node.hostContainer.zIndex = 3;
                        node.hostContainer.fillColor = '191,191,191';

                        node.hostNodes.forEach(function(n){
                            node.hostContainer.add(n);
                            n.zIndex = node.hostContainer.zIndex + 1;
                            addToScene(n);
                        });
                        scene.add(node.hostContainer);
                        node.hostContainer.x = event.x;
                        node.hostContainer.y = event.y;

                        node.hostLinks = [];
                        node.hostNodes.forEach(function(hostNode){
                            var link = new JTopo.Link(node, hostNode);
                            link.alpha = 0.4;
                            link.portZ = node.portA;
                            node.hostLinks.push(link);
                            scene.add(link);
                        });

                        var angle = node.angle || Math.PI/2;
                        if(node.hostNodes.length == 1){
                            var x = node.cx + Math.cos(angle) * 64;
                            var y = node.cy + Math.sin(angle) * 64;
                            node.hostNodes[0].x = x;
                            node.hostNodes[0].y = y;
                        }else{
                            var layout = getCircleLayout(node.hostNodes, {cx: node.cx, cy:node.cy,
                                beginAngle: angle - Math.PI/2,
                                endAngle: angle + Math.PI/2});
                            layoutNodes(node.hostNodes, layout.locations);
                            var x = node.cx + Math.cos(angle) * layout.radius - node.cx;
                            var y = node.cy + Math.sin(angle) * layout.radius - node.cy;
                            node.hostNodes.forEach(function(e){
                                e.x += x;
                                e.y += y;
                            });
                        }
                    });
                }
            }
        });

        // 定时获取动态增加、删除的节点
        function dynamicAddDelete(){
            try{
                Topo.loadDynamicDatas().done(function(response){
                    if(response.add && response.add.length > 0){
                        // 过滤掉那些已经存在的
                        var datas = response.add.filter(function(e){ return nodeMap[e.id] == null; });
                        if(datas && datas.length > 0){
                            var nodes = datas.map(switchToNode);
                            nodes.forEach(addToScene);
                            nodes.forEach(function(node){
                                if(isPhysical(node.data)){
                                    pyhsicalNodes.push(node);
                                }else if(isCoreSwitch(node.data)){
                                    coreSwitchNodes.push(node);
                                }else if(notCoreSwitch(node.data)){
                                    switchNodes.push(node);
                                }
                            });
                            layoutAll();
                            linkNodes(datas, scene, nodeMap, linkMap);

                            // 闪烁
                            Topo.highlightNodes(nodes, 10);
                        }
                    }
                    if(response["delete"] && response["delete"].length > 0){
                        var ids = response["delete"];
                        Topo.remove(Topo.circleStage, ids);
                    }
                });
            }catch(e){
                console.log('查询动态节点出错:' + e);
            }
            setTimeout(dynamicAddDelete, 3000);
        }
        setTimeout(dynamicAddDelete, 3000);
        //this.circleStage.centerAndZoom(); //自动缩放并居中
    }

    var Topo = {
        stage: null,
        treeStage: null,
        circleStage: null,
        treeData: null,
        circleData: null,
        treeNodeMap: {},
        circleNodeMap: {},
        initTree: initTree,
        initCircle: initCircle,
        /**
         * 设置当前布局
         * @param layout
         */
        setCurrentLayout: function(layout){
            if(layout == "tree"){
                this.stage = this.treeStage;
                if(this.circleStage){
                    this.circleStage.frames = 0;
                }
            }else{
                this.stage = this.circleStage;
                if(this.treeStage){
                    this.treeStage.frames = 0;
                }
            }
            if(this.stage){
                this.stage.frames = 24;
            }
        },
        getScene: function(){
            return this.stage.childs[0];
        },
        /**
         * 序列化 [{id:"", x:"", y:""}]
         * @param layout tree|
         */
        serilize: function(){
            var layout = this.stage === this.treeStage ? 'tree' : 'circle';
            var datas = this.stage === this.treeStage ? this.treeData.nodes : this.circleData.nodes;
            var json = '{';
            json += '"layout":"'+ layout+'"';
            json += ',"nodes":[';
            for(var i=0; i<datas.length; i++){
                var obj = datas[i];
                var node = this.getNodeById(obj.id);
                if(i > 0){
                    json+= ',';
                }
                json += '{';
                json += '"id":"'+obj.id+'",';
                json += '"x":"'+node.x+'",';
                json += '"y":"'+node.y+'"';
                json += '}';
            }
            json += ']';
            json += '}';
            return json;
        },
        // 获取所有当前选中的节点
        getSelectedNode: function(){
            return this.stage.childs[0].findElements(function(e){
                return e.selected == true;
            });
        },
        getNodeById: function(id){
            var map = (this.stage == this.treeStage) ? this.treeNodeMap : this.circleNodeMap;
            var node = map[id];
            return node;
        },
        getLink: function(aId, zId){
            var scene = this.stage.childs[0];
            var links = scene.findElements(function(e){
                if(! e instanceof JTopo.Link) return false;
                return e.nodeA && e.nodeZ && e.nodeA.id == aId && e.nodeZ.id == zId;
            });
            if(links.length > 0){
                return links[0];
            }else{
                //console.log('找不到指定的link: aId:' + aId + ' zId:' + zId);
                return null;
            }
        },
        getAllLink: function() {
            return this.stage.find('link');
        },
        getAllNode: function() {
            return this.stage.find('node');
        },
        // 选中指定节点
        selecteNode: function(id){
            var node = this.getNodeById(id);
            node.selected = true;
        },
        /**
         * 高亮闪烁节点
         * @param {Array} nodes
         */
        highlightNodes: function(nodes, times, delay){
            var self = this;
            times = times || 6; // 默认闪烁6次
            delay = delay || 500; // 闪烁间隔
            if(nodes ==  null || nodes.length == 0) return;
            if(typeof nodes[0] == 'string'){
                nodes = nodes.map(function(id){
                    return self.getNodeById(id);
                });
            }
            function nodeFlash(node, n){
                if(n == 0) {
                    //node.selected = false;
                    node.borderWidth = 0;
                    return;
                };
                //node.selected = !node.selected;
                node.borderWidth = node.borderWidth == 4 ? 0:4;
                node.borderColor = '255,0,0';
                setTimeout(function(){
                    nodeFlash(node, n-0.5);
                }, delay);
            }
            nodes.forEach(function(node){
                nodeFlash(node, times);
            });
        },
        /**
         * 取消所有高亮
         */
        cancleHighlightAll: function(){

        },
        /**
         * 加粗指定节点之间的所有连线
         * @param {Array} ids
         */
        boldLink : function(ids){
            for(var i=0; i<ids.length-1; i++){
                var link = this.getLink(ids[i], ids[i+1]);
                if(link == null){
                    link = this.getLink(ids[i+1], ids[i]);
                }
                if(link == null) continue;
                link.lineWidth_bak = link.lineWidth;
                link.alpha = 1;
                link.lineWidth = 6;
            }
        },
        /**
         * 取消所有加粗连线
         */
        cancleBoldAllLink : function(){
            this.getAllLink().forEach(function(link){
                if(link.lineWidth_bak){
                    link.lineWidth = link.lineWidth_bak;
                    delete link.lineWidth_bak;
                }
            });
        },
        // 改变两个节点之间的连线颜色
        changeLinkColor: function(aId, zId, color){
            var link = this.getLink(aId, zId);
            if(link){
                link.strokeColor = color;
            }
        },
        // 显示指定link的上下行带宽
        showLinkMonitor: function(aId, zId, color){
            //this.hideLinkMonitor(aId, zId);
            function addUpDownLink(stage){
                var isUP = false;
                var links = stage.find('link');
                var link = null;
                for(var i=0; i<links.length; i++){
                    var e = links[i];
                    if(e.nodeA && e.nodeZ){
                        if(e.nodeA.id == aId && e.nodeZ.id == zId){
                            isUP = false;
                            link = e;
                            break;
                        }else if(e.nodeZ.id == aId && e.nodeA.id == zId){
                            isUP = true;
                            link = e;
                            break;
                        }
                    }
                }
                if(link == null) return;
                var upLink = link.upLink;
                if(upLink == null){
                    upLink = new JTopo.Link(link.nodeA, link.nodeZ);
                    upLink.portA = link.portA;
                    upLink.portZ = link.portZ;
                    upLink.bundleGap = 2;
                    upLink.lineWidth = 4;
                    upLink.alpha=0;
                    upLink.strokeColor = '0,255,0';
                    upLink.nodeIndex = 0;
                    link.upLink = upLink;
                    stage.childs[0].add(upLink);
                }

                var downLink = link.downLink;
                if(downLink == null){
                    downLink = new JTopo.Link(link.nodeA, link.nodeZ);
                    downLink.portA = link.portZ;
                    downLink.portZ = link.portA;
                    downLink.bundleGap = 2;
                    downLink.lineWidth =4;
                    downLink.alpha=0;
                    downLink.strokeColor = '0,255,0';
                    link.downLink = downLink;
                    stage.childs[0].add(downLink);
                    downLink.nodeIndex = 2;
                }
                link.nodeIndex = 1;

                if(isUP){
                    upLink.alpha = 0.4;
                    upLink.strokeColor = color;
                }else{
                    downLink.alpha = 0.4;
                    downLink.strokeColor = color;
                }
            }
            addUpDownLink(this.treeStage);
            addUpDownLink(this.circleStage);
        },
        // 隐藏指定link的上下行带宽
        hideLinkMonitor: function(aId, zId){
            function removeUpDownLink(stage){
                var isUP = false;
                var links = stage.find('link');
                var link = null;
                for(var i=0; i<links.length; i++){
                    var e = links[i];
                    if(e.nodeA && e.nodeZ){
                        if(e.nodeA.id == aId && e.nodeZ.id == zId && e.upLink){
                            e.downLink.alpha = 0;
                            break;
                        }else if(e.nodeZ.id == aId && e.nodeA.id == zId && e.downLink){
                            e.upLink.alpha = 0;
                            break;
                        }
                    }
                }
            }
            removeUpDownLink(this.treeStage);
            removeUpDownLink(this.circleStage);
        },
        // 隐藏所有link的上下行带宽
        hideAllLinkMonitor: function(){
            function removeUpDownLink(stage){
                stage.find('link').forEach(function(link){
                    if(link.upLink){
                        stage.childs[0].remove(link.upLink);
                        delete link.upLink;
                    }
                    if(link.downLink){
                        stage.childs[0].remove(link.downLink);
                        delete link.downLink;
                    }
                    link.nodeIndex = 0;
                });
            }
            removeUpDownLink(this.treeStage);
            removeUpDownLink(this.circleStage);
        },
        // 让指定节点居中
        centerNode: function(id){
            var node = this.getNodeById(id);
            if(node){
                this.stage.setCenter(node.cx, node.cy);
            }
        },
        mousedown: function(f){
            this.treeStage.childs[0].mousedown(f);
            this.circleStage.childs[0].mousedown(f);
        },
        mousemove: function(f){
            this.treeStage.childs[0].mousemove(f);
            this.circleStage.childs[0].mousemove(f);
        },
        click: function(f){
            this.treeStage.childs[0].click(f);
            this.circleStage.childs[0].click(f);
        },
        dbclick: function(f){
            this.treeStage.childs[0].dbclick(f);
            this.circleStage.childs[0].dbclick(f);
        },
		mouseover: function(f){
            this.treeStage.childs[0].mouseover(f);
            this.circleStage.childs[0].mouseover(f);
        },
		mouseout: function(f){
            this.treeStage.childs[0].mouseout(f);
            this.circleStage.childs[0].mouseout(f);
        },
        exportImage: function(){
            this.stage.saveImageInfo();
        },
        // 和节点无关的link淡化掉
        highlightNodeLinks: function(nodeId){
            var links = this.stage.find('link');
            if(nodeId == null){
                links.forEach(function(link){
                    link.alpha = 0.4;
                });
            }else{
                links.forEach(function(link){
                   if(link.nodeA.id == nodeId || link.nodeZ.id == nodeId){
                       link.alpha = 1;
                   }else{
                       link.alpha = 0.4;
                   }
                });
            }
        },
        // 所有link淡化掉
        fadeAllLinks: function(){
            try{
            var links = this.stage.find('link');
            links.forEach(function(link){
                link.alpha = 0.03;
            });
            }catch(e){};
        },
        // 动态删除节点
        remove: function(stage, idArray){
            var scene = stage.childs[0];
            var nodes = scene.find('node');
            if(idArray && idArray.length > 0){
                nodes.forEach(function(node){
                    idArray.forEach(function(id){
                        if(node.id == id){
                            console.log("删除: " + node.text);
                            scene.remove(node);
                        }
                    });
                });
            }
        },
        zoomNodes: function(s){
            var nodes = this.getAllNode();
            nodes.forEach(function(n){
                n.x *= s;
                n.y *= s;
            });
        }
};
    exports.Topo = Topo;
})( window);

(function($, exports){
    //刷新频率获得，对应接口 4
    var base_path = "../topology/";

    var topo_path_interval_load=base_path+'interval_read.json';
    //刷新频率保存，对应接口 5
    var topo_path_interval_save=base_path+'interval_save.json';
    //交换机详细信息 对应接口 7
    var topo_path_detail_exchange=base_path+'detail_exchange.json';
    //带宽详细信息 对应接口 8 和 11
    var topo_path_detail_rate=base_path+'detail_rate.json';
    //fabric查看
    var topo_path_fabric=base_path+'link_fabric.json';
    var topo_path_fabric_set=base_path+'link_fabric_set.json';
    //主机详细信息获得 对应接口 10
    var topo_path_detail_host=base_path+'detail_host.json';
    //带宽监控实时获得 对应接口 6
    var topo_path_rate_used=base_path+'rate_used.json';
    //虚拟网数据加载 对应接口 12
    var topo_path_vitural_net='/controller/web/group/'+'main';
    //保存虚拟网数据  对应接口 13
    var topo_path_virtual_net_save='/controller/web/group/'+'group';

    //加载虚拟网下属节点 对应接口 14
    var topo_path_virtual_net_items='/controller/web/group/'+'groupDetails';
    //删除虚拟网元素 对应接口 16
    var topo_path_virtual_net_item_del=base_path+"virtual_net_item_del.json";
    //添加节点到虚拟网中 对应接口 15
    var topo_path_virtual_net_item_add='/controller/web/group/'+'node/add';

    //检索 对应接口 20
    var topo_path_search=base_path+"search_results.json";

    //最优路径 对应接口 17
    var topo_path_optimal_path =base_path+"path_optimal.json";
    //获得所有路径 对应接口 18
    var topo_path_optimal_path_all =base_path+"path_optimal_all.json";
    //保存最佳路径 对应接口 19
    var topo_path_optimal_path_save=base_path+"path_optimal_save.json";


    //交换机数据
    var exchange_tree = base_path+'exchange_tree.json';
    var exchange_circle =base_path+ 'exchange_circle.json';

    // 获取指定交换机下的服务器列表
    var exchange_host = base_path+'data/exchange_host.json';

    //保存当前交换机调整后的TOPO位置
    var exchange_topo_save=base_path+'data/exchange_save.json';



    //绿色，黄色，红色，深红
    var link_colors = ['4,220,9','255,255,0','255,150,150','255,0,0'];

    var rate_used = [0,1,2,3];

    //动态增加节点、删除节点
    var dynamic_add_delete = base_path+'data/dynamic_add_delete.json';

    var refresh_Interval = 200;  //带宽占用刷新时间间隔
    var lastCheckedNode;//最后选择的节点

    //刷新频率设置弹出窗
    var refreshSettingModal;
    //创建组弹出窗口
    var virtualNetCreateModal;

    var currentLayout = 'tree';//当前布局

    var currentVirtualNetId;//当前虚拟网ID
    var lastPopShowContent;//内容弹出框，最后显示的内容，可能是 虚拟网节点 vn，检索结果 rs
    //
    var ipp="";
    var portt="";
    function initAll(){
        initUI();

        Topo.loadHostDatas = function(switchId){
            return $.ajax({ url: exchange_host+"?ip="+ipp+"&port="+portt+"switch_id=" + switchId});
        };
        Topo.loadDynamicDatas = function(){
            //return $.ajax({url: dynamic_add_delete});
        };
        // 初始化Canvas内容
        ipp =$("#control").val().split("|")[0];
        portt =$("#control").val().split("|")[1];
        $.when($.ajax({ url: exchange_tree+"?ip="+ipp+"&port="+portt}), $.ajax({url: exchange_circle+"?ip="+ipp+"&port="+portt})).done(function(treeData, circleData){
            Topo.initTree(treeData[0], $('#canvas_tree')[0]);
            Topo.initCircle(circleData[0], $('#canvas_circle')[0]);

            Topo.mousemove(function(event){

                if(this.mouseTimer){
                    //console.log('Window.clearTimeout(this.mouseTimer);');
                    clearTimeout(this.mouseTimer);
                }
                if(event.target == null){
                    mouseover_event = null;
                    last_show_detail_node = null;
                    $('#detailDiv').hide();
                    return;
                }
                mouseover_event = event;
                //
                this.mouseTimer = setTimeout("showDetail();",1000);
            });


            Topo.mousedown(function(event){
                $('#rateDetail').hide();
                $('#detailDiv').hide();
            });

            //添加淡化处理


            Topo.click(function(event){
                if(! $("#pathCheck").is(':checked')){
                    Topo.cancleBoldAllLink();//取消所有加粗
                }

                if(event.target == null){
                    lastCheckedNode = null;
                    $('#pathMenu').hide();
                    if(! $("#pathCheck").is(':checked')){
                        Topo.highlightNodeLinks();
                    }
                    return;
                }
                var e = event.target;
                if(e instanceof JTopo.Node){
                    Topo.cancleBoldAllLink();//取消所有加粗

                    //如果还未点击过节点，则记录节点
                    if(lastCheckedNode == null){
                        lastCheckedNode = e;
                    }
                    Topo.highlightNodeLinks(e.id);
                }else{
                    Topo.highlightNodeLinks();
                }
                console.log('Topo.click '+event.button);
                //if(event.button == 1){//左键.
                    console.log('event.button 2 '+event.target);

                showPath(event);

                //}
                /**
                if(event.button == 2){// 右键

                    if(event.target == null){
                        lastCheckedNode = null;
                        return;
                    }
                    var e = event.target;
                    if(e instanceof JTopo.Node){
                        lastCheckedNode = e;

                        // 弹出右键菜单（div）
                        $("#contextmenu").css({
                            top: event.pageY,
                            left: event.pageX
                        }).show();
                    }
                }**/
            });


            if(getCurrentLayout() == 'circle'){
                showCircleCanvas();
            }
        });

        //事件处理初始化
        initEvent();

        //将canvas的大小，重置为跟父div一样
        resizeCanvas();

        loadInterval();
        //loadVirtualNet();

        //加载带宽监控
        loadRateUsedDataThread();
    }


    //初始化界面
    function initUI(){
        //layoutALL();//总体布局
        //initSearchForm();//搜索界面
        //initCanvasPanel();//右侧topo
        initContextMenu();//右键菜单
        //加载虚拟网UI
       var html = initVirtualNetPanel();
        $(document.body).append(html);
    }

    //右键菜单
    function initContextMenu(){

        var html = '<ul id="contextmenu" class="modal" style="width:170px;position: absolute;list-style: none;margin: 0;padding: 0;display: none">	'
            +'<li><a id="tree" style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;">\u6811\u5f62\u5e03\u5c40\u5c55\u793a</a></li>' //树形布局展示
            +'<li><a id="grid" style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;">\u73af\u5f62\u5e03\u5c40\u5c55\u793a</a></li>' //环形布局展示
            +'<li><a style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;"><label>\u6846\u9009\u6a21\u5f0f&nbsp;<input id="selectModeCheck" type="checkbox"  /></label></a></li>' //是否显示详情
            +'<li><a style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;"><label>\u663e\u793a\u8be6\u60c5\u7a97\u53e3&nbsp;<input id="detailCheck" type="checkbox"  checked="checked" /></label></a></li>' //是否显示详情
            +'<li><a style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;"><label>\u663e\u793a\u6700\u4f18\u8def\u5f84&nbsp;<input id="pathCheck" type="checkbox" /></label></a></li>' //是否最优路径
            +'<li><a id="showVirtualNetBtn" style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;">\u865a\u62df\u7f51\u663e\u793a</a></li>' //虚拟网显示
            +'<li><a id="createVirtualNetBtn" style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;">\u65b0\u5efa\u865a\u62df\u7f51</a></li>' //新建虚拟网
            +'<li><a id="addToVirtualNet" style="display: block;padding: 10px;border-bottom: 1px solid #aaa;cursor: pointer;">\u6dfb\u52a0\u9009\u5b9a\u8282\u70b9\u5230\u865a\u62df\u7f51</a></li>' //添加选定节点到虚拟网
            +'</ul>';
        $(document.body).append(html);

        $(document).click(function(e){
            if(e.button!= 2){//右键
                $('#contextmenu').hide();
            }
            //隐藏详细窗口
            //$('#detailDiv').hide();
        });

        $('#canvas_circle').mouseup(function(e){
            console.log(e.button);
            if(e.button == 2){//右键
                $('#contextmenu').css({
                    left: e.pageX,
                    top: e.pageY
                });
                $('#contextmenu').show();
                return false;
            }
        });
        $('#canvas_tree').mouseup(function(e){
            console.log(e.button);
            if(e.button == 2){//右键
                $('#contextmenu').css({
                    left: e.pageX,
                    top: e.pageY
                });
                $('#contextmenu').show();
            }
        });
    }

    //布局右侧
    function layoutALL(){
        $('#right-top').css({"height":"100%"});
        $('#right-bottom').css({"height":0});
    }

    //搜索框
    function initSearchForm(){
        var html = '<div class="dash">'
            +'<ul class="nav nav-tabs">'
            +' <li class="active"><a id="connection" href="#">\u68c0\u7d22</a></li></ul>'
            +' <div class="dashlet row-fluid">'
            +' <form class="navbar-form" role="search">'
            +' <div class="btn-group">'
            +' <a name="all" class="searchtype btn btn-success">\u6240\u6709</a>'
            +' <a name="exchange" class="searchtype btn btn-default">\u4ea4\u6362\u673a</a>'
            +' <a name="host" class="searchtype btn btn-default">\u670d\u52a1\u5668</a>'
            +' <input type="hidden" id="searchType" value="all"/>'
            +' </div><a id="searchButton" class="btn btn-primary">\u68c0\u7d22</a>'
            +' <div class="form-group">'
            +' <input id="searchName" type="text" class="form-control span9" placeholder="nodeId">'
            +'</div> '
            +' <div class="form-group">'
            +' <input id="searchMac" type="text" class="form-control span9" placeholder="MAC\u5730\u5740">'
            +' </div>'
            +' <div class="form-group">'
            +' <input id="searchIp" type="text" class="form-control span9" placeholder="IP\u5730\u5740">'
            +' </div>'
            +' </form>'
            +' </div>'
            +' </div>';


        return html;
    }



    //工具栏
    function getToolBarHtml(){
        var html = ' <div id="toolbar" style="color: #00A8EC">'
            // +' <li id="tree" class="active" onclick=""><a id="staticRouteConfig" href="#">Tree</a></li>'
            // +' <li id="grid"  onclick=""><a id="" href="#">Circle</a></li>'
            // +' &nbsp;&nbsp;&nbsp;&nbsp;'
            // +' \u6700\u4f18\u8def\u5f84 &nbsp;<input id="pathCheck" type="checkbox" />' //最优路径
            // +' &nbsp;&nbsp;'
            // +' \u8be6\u60c5\u67e5\u770b&nbsp;<input id="detailCheck" type="checkbox"  checked="checked" />' //详情查看
            +' &nbsp;'
            +'<a style="padding:1px;font-family:SimHei" id="rateConfig" title="\u76d1\u63a7\u9891\u7387" class="btn btn-primary"><img style="padding-bottom: 3px;" width="16" height="16"  src="'+prefix+'img/setting.png"/>&nbsp;\u8bbe\u7f6e</a>'//监控频率
            +' &nbsp;'
            +'<a style="padding:1px;font-family: SimHei" id="topoSaveBtn" title="\u4fdd\u5b58\u62d3\u6251\u8bbe\u7f6e" class="btn btn-primary"><img style="padding-bottom: 3px;"  width="16" height="16"  src="'+prefix+'img/save.png"/>&nbsp;\u4fdd\u5b58</a>'//保存拓扑
            +'<input type="hidden" id="rateUsedMonitor" value="0"/>'
            +'&nbsp;&nbsp;&nbsp;&nbsp;'
            +' <a style="padding:1px;width: 50px;font-family: SimHei" id="searchTypeButton" name="all" class="btn btn-primary" >\u6240\u6709</a>' //全部
            +'&nbsp;&nbsp;<input id="searchName" style="width:150px;" placeholder="Id/MAC/ip">'
            //+'<input id="searchMac"  style="width:100px;" placeholder="MAC\u5730\u5740">'
            // +'<input id="searchIp"  style="width:100px;" placeholder="IP\u5730\u5740">'
            +'&nbsp;<a style="padding:1px;font-family: SimHei" id="searchButton" class="btn btn-primary" title="\u68c0\u7d22"><img style="padding-bottom: 3px;" width="16" height="16"  src="'+prefix+'img/nodesearch.png"/>&nbsp;\u68c0\u7d22</a>'
            +'&nbsp;&nbsp;<img id="rateUsedRadio" class="btn" src="'+prefix+'img/traffic-light-off.png" title="\u70b9\u51fb\u5f00\u542f\u76d1\u63a7" />'//监控是否开启
            +'<img id="resizeBtn" class="btn" src="'+prefix+'img/max_size.png" width="16" title="\u5168\u5c4f" height="16" style="padding-top:5px;margin-left:5px;"/>' //全屏
            +'</div>';

        var bunGroup =  ' <div id="searchTypeButtonGroup" style="display: none;position: absolute;" class="btn-group">'
            +' <a name="all" class="searchtype btn btn-success">\u6240\u6709</a>' //全部
            +' <a name="exchange" class="searchtype btn btn-default">\u4ea4\u6362\u673a</a>' //交换机
            +' <a name="host" class="searchtype btn btn-default">\u670d\u52a1\u5668</a>' //服务器
            +' <input type="hidden" id="searchType" value="all"/>'
            +' </div>';
        return html+bunGroup;
    }

    //获得两个canvas
    function getCanvasDivHtml(){
        var html = '<canvas width="800" height="600" id="canvas_tree" style="background-color: rgb(251, 251, 251);"></canvas>'
            +'<canvas width="800" height="600" id="canvas_circle" style="background-color: rgb(251, 251, 251);display:none;"></canvas>';
        return html;
    }

    //显示树形布局
    function showTreeCanvas(){
        document.getElementById('tree').className='active';
        document.getElementById('grid').className='';
        document.getElementById('canvas_tree').style.display='';
        document.getElementById('canvas_circle').style.display='none';
        Topo.setCurrentLayout('tree');
        currentLayout = 'tree';
        saveCurrentLayout();
    }

    //显示圆形布局
    function showCircleCanvas(){
        document.getElementById('grid').className='active';
        document.getElementById('tree').className='';
        document.getElementById('canvas_circle').style.display='';
        document.getElementById('canvas_tree').style.display='none';
        Topo.setCurrentLayout('circle');
        currentLayout = 'circle';
        saveCurrentLayout();
    }

    function saveCurrentLayout(){
        setCookie('current_vancas',currentLayout,200);
    }

    function getCurrentLayout(){
        return getCookie('current_vancas');
    }

    function setCookie(c_name,value,expiredays)
    {
        var exdate=new Date()
        exdate.setDate(exdate.getDate()+expiredays)
        document.cookie=c_name+ "=" +escape(value)+
            ((expiredays==null) ? "" : ";expires="+exdate.toGMTString())
    }

    function getCookie(c_name)
    {
        if (document.cookie.length>0)
        {
            c_start=document.cookie.indexOf(c_name + "=")
            if (c_start!=-1)
            {
                c_start=c_start + c_name.length+1
                c_end=document.cookie.indexOf(";",c_start)
                if (c_end==-1) c_end=document.cookie.length
                return unescape(document.cookie.substring(c_start,c_end))
            }
        }
        return ""
    }

    //初始化事件处理
    function initEvent(){
        //布局Tab切换
        //网状布局
        $('#tree').click(function(){
            showTreeCanvas();
        });
        //网状布局
        $('#grid').click(function(){
            showCircleCanvas();
        });

        //模式转换
        $('#selectModeCheck').click(function(){
            var mode = 'normal';
            if(this.checked){
                mode = 'select';
            }
            Topo.treeStage.mode = mode;
            Topo.circleStage.mode = mode;
        });

        //刷新频率设置
        $('#rateConfig').click(function(){
            var inputId = 'refresh_interval_input';
            if(!refreshSettingModal){
                refreshSettingModal = one.lib.modal.spawn('one.main.config.id', '\u5e26\u5bbd\u76d1\u63a7\u9891\u7387',
                        '<form class="navbar-form navbar-left" role="search">'
                        +' <div class="form-group">'
                        +' <input id="'+inputId+'" type="text" class="form-control" placeholder="\u6beb\u79d2" value="'+refresh_Interval+'"> '
                        +' \u6beb\u79d2'
                        +' &nbsp;&nbsp;&nbsp;&nbsp; <button type="submit" class="btn btn-default" onclick="window.updateInterval(document.getElementById(\''+inputId+'\').value);return false;">\u4fdd\u5b58\u8bbe\u7f6e</button> '
                        + ' </div></form>', 'foot');
            }
            $(refreshSettingModal).width(400);
            //$($modal).width(500);
            refreshSettingModal.modal();
            // $modal.modal('hide');
        });

        //topo保存
        $('#topoSaveBtn').click(function(){
            var data = Topo.serilize();
            $.post(exchange_topo_save,{layout:currentLayout,data:data},function(dataObj){
                alert(dataObj.message);
            });
        });

        $('#searchTypeButton').click(function(){
//            $('#searchTypeButtonGroup').css({
//                top:$(this).offset().top,
//                left:$(this).offset().left
//            });
            $('#searchTypeButtonGroup').show();
            return false;
        });

        $(document).click(function(){
            $('#searchTypeButtonGroup').hide();
        });

        //查询选项组
        $('.searchtype').click(function(){
            $(this).parent().children('.searchtype').attr('class','searchtype btn btn-default');
            $('#searchType').val($(this).attr('name'));
            $(this).attr('class','searchtype btn btn-success');
            $('#searchTypeButton').html($(this).html());
            return false;
        });

        //查询按钮
        $('#searchButton').click(function(){
            doSearch();
        });

        //创建虚拟网
        $('#createVirtualNetBtn').click(function(){
            createVirtualNet();
        });

        //虚拟网显示
        $('#showVirtualNetBtn').click(function(){
            showVirtualNetPanel();
        });

        //添加到虚拟网
        $('#addToVirtualNet').click(function(){
            var selectNodes = Topo.getSelectedNode();
            if(selectNodes.length > 0){
                if(!$('#MyTopoPopWindows').is(":visible")){
                    alert('\u672a\u9009\u4e2d\u5bf9\u5e94\u865a\u62df\u7f51\uff0c\u8bf7\u9009\u62e9\u5177\u4f53\u865a\u62df\u7f51\u8fdb\u5165\u8be6\u60c5'); //未选中对应虚拟网，请选择具体虚拟网进入详情
                    return;
                }
                console.log('lastPopShowContent:'+lastPopShowContent+',currentVirtualNetId:'+currentVirtualNetId);
                if(lastPopShowContent == 'vn'){
                    var ids = '';
                    for(var i=0;i<selectNodes.length;i++){
                        ids=ids+','+selectNodes[i].id;
                    }
                    $.post(topo_path_virtual_net_item_add,{group:currentVirtualNetId,nodeId:ids},function(dataObj){
                        if(dataObj.status == 'true'){
                            //加载对应详细信息
                            /**
                            var addedNode = lastCheckedNode;
                            var url = topo_path_detail_exchange;
                            var paraName = 'nodeId';
                            if(addedNode.type == 'host'){
                                url = topo_path_detail_host;
                                paraName = 'hostId';
                            }
                            //加载详情
                            $.get(url+'?'+paraName+'='+addedNode.id,function(dataObj){
                                $('#MyTopoPopWindows table tbody').prepend(createVirtualItemHtml(currentVirtualNetId,lastCheckedNode.id,lastCheckedNode.type,lastCheckedNode.text,dataObj.ip,dataObj.mac));
                            });
                             **/
                            for(var i=0;i<selectNodes.length;i++){
                                var node = selectNodes[i];
                                $('#MyTopoPopWindows table tbody').prepend(createVirtualItemHtml(currentVirtualNetId,node.id,node.type,node.text,node.ip,node.mac));
                            }

                            //alert(dataObj.message);
                        }else{
                           // alert(dataObj.message);
                        }
                    });
                }
            }else{
                alert('\u5f53\u524d\u65e0\u8282\u70b9\u9009\u4e2d,\u8bf7\u5de6\u952e\u70b9\u51fb\u9009\u62e9\u8282\u70b9'); //当前无节点选中,请左键点击选择节点
            }
        });

        //全屏
        $('#resizeBtn').click(function(){
           fullTopoInWin();
        });


        //是否开启实时监控
        $('#rateUsedRadio').click(function(){
            if($('#rateUsedMonitor').val() == 0){//开启监控
                $('#rateUsedRadio').attr('title','\u70b9\u51fb\u53d6\u6d88\u76d1\u63a7');
                $('#rateUsedRadio').attr('src',prefix+'img/traffic-light.png');
                $('#rateUsedMonitor').attr('value',1);
                //$('#rateUsedRadio').attr('class','btn btn-primary');;
            }else{//关闭监控
                $('#rateUsedRadio').attr('title','\u70b9\u51fb\u5f00\u542f\u76d1\u63a7');
                $('#rateUsedRadio').attr('src',prefix+'img/traffic-light-off.png');
                $('#rateUsedMonitor').attr('value',0);
                //$('#rateUsedRadio').attr('class','btn');;
                Topo.hideAllLinkMonitor();
            }
        });


    }

    //创建虚拟网
    function createVirtualNet(){
        if(!virtualNetCreateModal){
            virtualNetCreateModal = one.lib.modal.spawn('one.main.config.id', '\u521b\u5efa\u865a\u62df\u7f51',
                    '<form class="navbar-form navbar-left" role="search">'
                    +'<div class="form-group">\u540d\u5b57: <input id="input_virtual_net_name" type="text" class="form-control" placeholder="\u552f\u4e00\u540d\u79f0" ></div>  '
                    +'<div class="form-group">\u663e\u793a: <input id="input_virtual_net_text" type="text" class="form-control" placeholder="\u663e\u793a\u540d" ></div>  '
                    +'<div class="form-group">\u63cf\u8ff0: <input id="input_virtual_net_desc" type="text" class="form-control" placeholder="\u63cf\u8ff0" ></div>  '
                    + '<div class="form-group"><br/><button type="submit" class="btn btn-default" onclick="window.saveVirtualNet();return false;">\u4fdd\u5b58</button></div> </form>', 'foot');
        }
        $(virtualNetCreateModal).width(300);
        virtualNetCreateModal.modal();
    }

    //保存虚拟网
    function saveVirtualNet(){
        var name = $('#input_virtual_net_name').val();
        var text = $('#input_virtual_net_text').val();
        var desc = $('#input_virtual_net_desc').val();

        $('#input_virtual_net_name').val('');
        $('#input_virtual_net_text').val('');
        $('#input_virtual_net_desc').val('');
        $.post(topo_path_virtual_net_save,{name:name,text:text,desc:desc},function(dataObj){
            if(dataObj.status == 'true'){
                alert(dataObj.message);
                $('#virtual_body').prepend('<tr style="cursor: default;" onclick="showVirtualNetAllNode(\''+obj.group+'\')"  id="col_'+name+'"><td>'+text+'</td><td><a id="'+name+'" onclick="window.loadVirtualNetItems(this.id);" class="btn groupAjust">详情</a></td></tr>');
                virtualNetCreateModal.modal('hide');
            }else{
                alert(dataObj.message);
            }
        });
    }

    //虚拟网加载
    function initVirtualNetPanel(){
        var html = '<div id="virtualNetBody" class="dash" style="display: none;"> '
           // +' <ul class="nav nav-tabs"> '
           // + '<li class="active"><a id="SearchPanel" href="#">\u865a\u62df\u7f51</a></li></ul> '
            +' <div class="dashlet row-fluid"> '
            +' <table class="table table-hover"> '
            +' <thead>'
            +' <tr>'
            +' <th>\u540d\u79f0</th>'
            +' <th>\u64cd\u4f5c</th>'
            +' </tr>'
            +' </thead>'
            +' <tbody id="virtual_body">'
            +'</tbody>'
            +'</table>'
            +'</div>'
            +'</div>';
        return html;
    }

    //虚拟网加载
    function loadVirtualNet(){
        $.get(topo_path_vitural_net,function(dataObj){
            for(var i=0;i<dataObj.length;i++){
                var obj = dataObj[i];
                $('#virtual_body').append('<tr  style="cursor: default;" onclick="showVirtualNetAllNode(\''+obj.group+'\')" id="col_'+obj.group+'"><td>'+obj.text+'</td><td><a id="'+obj.group+'" onclick="window.loadVirtualNetItems(this.id);" class="btn groupAjust">详情</a></td></tr>');
            }
            //虚拟网
            $('.groupAjust').click(function(){
                $('#virtualGroupDetail').width($('#left').width()-25);
                $('#virtualGroupDetail').show();
            });
        });
    }

    /**
     * 在拓扑中显示所有节点
     * @param name
     */
    function showVirtualNetAllNode(name){
        $.get(topo_path_virtual_net_items+'?group='+name,function(dataObj){
            var items = dataObj.items;
            var ids = new Array();
            for(var i=0;i<items.length;i++){
                var item = items[i];
                ids.push(item.nodeId);
            }

            Topo.centerNode(items[0].nodeId);
            Topo.highlightNodes(ids,6);
        });
    }

    //加载虚拟网成员
    function loadVirtualNetItems(name){
        $.get(topo_path_virtual_net_items+'?group='+name,function(dataObj){
            currentVirtualNetId = name;
            lastPopShowContent = 'vn';
            var title = name;
            var bodyContent =   '';
            var items = dataObj.items;
            for(var i=0;i<items.length;i++){
                var item = items[i];
                bodyContent = bodyContent+createVirtualItemHtml(dataObj.group,item.nodeId,item.type,item.text,item.ip,item.mac);
            }
            var bodyAll = '<table class="table table-hover"><thead><tr><th>\u7c7b\u578b</th><th>\u540d\u79f0</th><th>IP</th><th>MAC</th><th>\u64cd\u4f5c</th></tr></thead>'
                +'<tbody>'
                +bodyContent
                +'</tbody></table>';
            var htm = createMyTopoPopWin(title,bodyAll);
            $('#MyTopoPopWindows').remove();
            $(document.body).append(htm);
            $('#MyTopoPopWindows').width(500);
            $('#MyTopoPopWindows').show();

            /**
             * 将所有节点高亮
             */
            var nodeIds = new Array();
            for(var i=0;i<items.length;i++){
                var item = items[i];
                nodeIds.push(item.nodeId);
            }
            console.log('hight light nodes:'+nodeIds);
            Topo.highlightNodes(nodeIds,6);
        });
    }
    function createVirtualItemHtml(group,id,type,text,ip,mac){
        var html = '<tr onclick="centerNode(\''+id+'\')" style="cursor: default" id="virtual_item_'+id+'" ><td>'+type+'</td>'
            +'<td>'+text+'</td>'
            +'<td>'+ip+'</td>'
            +'<td>'+mac+'</td>'
            +'<td><a id="'+id+'" onclick="window.delVirtualNetItem(\''+group+'\',\''+id+'\');" title="\u5220\u9664\u5f53\u524d\u8282\u70b9" class="btn">X</a></td></tr>'; //删除当前节点
        return html;
    }

    /**
     * 居中显示指定节点
     * @param nodeId
     */
    function centerNode(nodeId){
        console.log('centfor show node:'+nodeId);
        Topo.centerNode(nodeId);
        Topo.highlightNodes([nodeId],6,0);
    }

    //显示虚拟网
    function showVirtualNetPanel(){
        var title = "\u865a\u62df\u7f51\u5217\u8868";
        var bodyAll = $('#virtualNetBody').html();
        var htm = createMyTopoPopWin(title,bodyAll);
        $('#MyTopoPopWindows').remove();
        $(document.body).append(htm);
        $('#MyTopoPopWindows').width(500);
        $('#MyTopoPopWindows').show();
    }

    //检索，加载结果
    function doSearch(){
        var type = $('#searchType').val();
        var nodeId = $('#searchName').val().trim();
        //var mac=$('#searchMac').val().trim();
        //var ip=$('#searchIp').val().trim();
        var mac=nodeId;
        var ip=nodeId;

        var nodes = Topo.getAllNode();
        var results = new Array();
        for(var i=0;i<nodes.length;i++){
            var n = nodes[i];
            if(type != 'all'){
                if(type == 'exchange'){//只检索交换机
                    if(n.type != 'switch'){
                        continue;
                    }
                }else
                if(type == 'host'){//只检索服务器
                    if(n.type != 'host'){
                        continue;
                    }
                }
            }

            var data = n.data;

            //如果nodeId不为空，且匹配不成功，则继续循环
            if((nodeId != '' && n.id.indexOf(nodeId)==-1)
                ||(data.ip && ip != '' && data.ip.indexOf(ip) == -1)
                ||data.mac && mac != '' && data.mac.indexOf(mac) == -1){
                continue;
            }

            results.push(n);
        }

        lastPopShowContent = 'rs';
        var title = "\u67e5\u5230\u7b26\u5408\u6761\u4ef6\u7684\u8282\u70b9\u6570: <font color=\"red\">"+results.length+"</font>";
        var bodyContent =   '';
        var items = results;
        for(var i=0;i<items.length;i++){
            var item = items[i];
            bodyContent = bodyContent+'<tr onclick="centerNode(\''+item.id+'\')" style="cursor:default;" id="virtual_item_'+item.id+'"><td>'
                +item.type+'</td>'
                +'<td>'+item.id+'</td>'
                +'<td>'+item.text+'</td>'
                +'</tr>'
        }
        var bodyAll = '<table class="table table-hover"><thead><tr><th>\u7c7b\u578b</th><th>ID</th><th>\u540d\u79f0</th></tr></thead>'
            +'<tbody>'
            +bodyContent
            +'</tbody></table>';
        var htm = createMyTopoPopWin(title,bodyAll);
        $('#MyTopoPopWindows').remove();
        $(document.body).append(htm);
        $('#MyTopoPopWindows').width(450);
        $('#MyTopoPopWindows').show();

    }

    //删除虚拟网成员
    function delVirtualNetItem(group,nodeId){
        $.post(topo_path_virtual_net_item_del,{},function(dataObj){
            if(dataObj.status == 'true'){
                alert(dataObj.message);
                $('#virtual_item_'+nodeId).remove();
            }else{
                alert(dataObj.message);
            }
        });

    }

    function createMyTopoPopWin(title,body){
        var top =40;
        var left =285;
        var html = '<div class="modal" id="MyTopoPopWindows" style="position:absolute;left:'+left+'px;top:'+top+'px;width:360px;z-index:99999;display:none"> '
            +' <div class="modal-header">'
            +' <button type="button" class="close" onclick="document.getElementById(\'MyTopoPopWindows\').style.display=\'none\'" data-dismiss="modal" aria-hidden="true">x</button>'
            +' <h3>'+title+'</h3> '
            +' </div> '
            +' <div class="modal-body">'
            +body
            +' </div> <div class="modal-footer"></div></div>';
        return html;
    }



    //交换机详细信息
    function loadExchangeDetail(event){
        var leftGap = 25;
        var topGap = 15;
        var e = event.target;
        var dataObj = e.data;
        var nodesHtml = '';

        var upNode = '';
        if(dataObj.upNode.length == 0){
            upNode = '\u65e0';//无上行
        }
        for(var i=0;i<dataObj.upNode.length;i++){
            var upObj = dataObj.upNode[i];
            if(typeof upObj == 'string'){
                upObj = {id: upObj, portA:'', portZ:''};
            }
            upNode=upNode+upObj.id+"<br/>";
            nodesHtml= nodesHtml+'<tr><td colspan="3">'+upObj.id+'</td></tr>';
        }
        var downNode = '';
        if(dataObj.downNode.length == 0){
            downNode = '\u65e0';//无下行
        }
        for(var i=0;i<dataObj.downNode.length;i++){
            var downObj = dataObj.downNode[i];
            if(typeof downObj == 'string'){
                downObj = {id: downObj, portA:'', portZ:''};
            }
            downNode=downNode+downObj.id+"<br/>";
            nodesHtml= nodesHtml+'<tr><td colspan="3">'+downObj.id+'</td></tr>';
        }

        var contentTable = '<li><table width="100%" id="rateDetailTable">'
            +'<tr><td width="60">ID:</td><td colspan="2" align="left">'+ dataObj.id+'</td></tr>' //
            +'<tr><td>IP:</td><td colspan="2" align="left">'+ dataObj.ip+'</td></tr>' //
            +'<tr><td>MAC:</td><td colspan="2" align="left">'+ dataObj.mac+'</td></tr>' //
            +'<tr><td colspan="3"><table class="table table-condensed">'
            +'<thead><tr><td>\u5173\u8054\u8282\u70b9</td><td></td><td></td></tr></thead><tbody>' //传输率
            +nodesHtml
            +'</tbody></table></td></tr>'
            +'</table></li>';

        var title = '\u4ea4\u6362\u673a\u8be6\u60c5';
        var div = $("#detailDiv");
        if(!div.attr('id')){
            var html = createDetailWin('detailDiv','Exchange Detail',contentTable,event);
            $(document.body).append(html);
        }else{
            $('#detailDiv ul li').remove();
            $('#detailDiv ul').append(contentTable);
        }
        $('#detailDiv').css({
            top: event.pageY+topGap,
            left: event.pageX+260+leftGap,
            width:280
        });
        $('#detailDiv h4').html(title);
        $('#detailDiv').show();

    }


    //主机详情
    function loadHostDetail(event){
        var leftGap = 25;
        var topGap = 15;
        var e = event.target;
        var dataObj = e.data;

        var content = '<li><table width="100%" id="rateDetailTable">'
            +'<tr><td width="60">ID:</td><td colspan="2" align="left">'+ dataObj.id+'</td></tr>' //
            +'<tr><td>IP:</td><td colspan="2" align="left">'+ dataObj.ip+'</td></tr>' //
            +'<tr><td>MAC:</td><td colspan="2" align="left">'+ dataObj.mac+'</td></tr>' //
            +'<tr><td>\u7ba1\u7406\u7aef\u53e3:</td><td colspan="2" align="left">'+ dataObj.port+'</td></tr>' //
            +'</table></li>';


        var div = $("#detailDiv");
        var title = '\u670d\u52a1\u5668\u8be6\u60c5';
        if(!div.attr('id')){
            var html = createDetailWin('detailDiv',title,content,event);
            $(document.body).append(html);
        }else{
            $('#detailDiv ul li').remove();
            $('#detailDiv ul').append(content);
        }
        $('#detailDiv h4').html(title);
        $('#detailDiv').css({
            top: event.pageY+topGap,
            left: event.pageX+260+leftGap
        });
        $('#detailDiv').show();
    }

    //创建详细信息窗口
    function createDetailWin(id,title,content){
        var leftGap = 50;
        var topGap = 15;
        var width = 260;
        var html = '<div class="modal" id="'+id+'" style="position:absolute;width:'+width+'px;z-index:99999;display:none;">'
            + ' <div class="modal-header">'
            +' <h4>'+title+'</h4>'
            +' </div>'
            +' <div class="modal-body">'
            +' <ul id="detail_div_content" style="list-style-type:none;margin:0 0 10px 2px;">'
            +content
            +' </ul>'
            +' </div>'
            +' </div>';
        return html;
    }

    //加载刷新频率
    function loadInterval(){
        $.ajax({ url: topo_path_interval_load+"?ip="+ipp+"&port="+portt, context: document.body, success: function(data){
            refresh_Interval = data.interval;
        }});
    }

    //设置刷新频率
    function updateInterval(interval){
        //alert(interval);
        $.post(topo_path_interval_save,{interval:interval,ip:ipp, port:portt},function(dataObj){
            if(dataObj.status == 'true'){
                alert(dataObj.message);
                refresh_Interval = interval;
                refreshSettingModal.modal('hide');
            }else{
                alert(dataObj.message);
            }
        });
    }

    /**
     * 重置canvas大小
     */
    function resizeCanvas(){
        $('#canvas_tree').attr('width',$('#chart').width());
        $('#canvas_tree').attr('height',$('#chart').height()-50);
        $('#canvas_circle').attr('width',$('#chart').width());
        $('#canvas_circle').attr('height',$('#chart').height()-50);
    }

    //动态调整topo大小
    $(window).resize(function(){
        resizeCanvas();
    });

    //详情查看
    var last_show_detail_node = null;
    var mouseover_event = null;
    function showDetail(){
        var event = mouseover_event;
        console.log('showDetail:'+event.target);
        var leftGap = 100;
        var topGap = 15;

        if(event.target == null || (!$("#detailCheck").is(':checked'))){
            $('#rateDetail').hide();
            $('#detailDiv').hide();
            last_show_detail_node = null;
            return;
        }
        var e = event.target;
        if(e instanceof JTopo.Node){
            if(e.type == 'host'){//根据节点类型，显示对应的详细信息
                if(last_show_detail_node != e){
                    loadHostDetail(event);
                }
            }else{
                if(last_show_detail_node != e){
                    loadExchangeDetail(event);
                }
            }
        }else
        if(e instanceof JTopo.Link){
            if(last_show_detail_node != e){
                loadRateDetail(event);
            }
        }


        last_show_detail_node = e;
    }

    //最优路径查看
    function showPath(event){
        $('#pathMenu').hide();//隐藏更多路径选择菜单
        var e = event.target;
        if(e instanceof JTopo.Node && $("#pathCheck").is(':checked')){
            Topo.fadeAllLinks();//淡化所有节点
            //如果还未点击过节点，则记录节点
            if(lastCheckedNode == null){
                lastCheckedNode = e;
            }else if(lastCheckedNode != e){
                //lastCheckedNode.selected = true;
                //e.selected = true;
                //加粗路径
                var nodeA = lastCheckedNode;
                var nodeB = e;

                //加载最优路径
                loadOptimalPath(nodeA,nodeB);
                //显示菜单，提醒可以选择更多路径
                //addPathMenu();
                $('#pathMenu').css({
                    top: event.pageY + 17,
                    left: event.pageX +287
                });

                $('#pathMenu').show();
                $('#pathMenu').unbind("click");//移除上次绑定的事件，防止重复绑定
                $('#pathMenu').click(function(e){
                    //显示所有路径列表
                    showAllPath(nodeA,nodeB);
                    $('#pathMenu').hide();
                });
               // lastCheckedNode = null;
            }
        }
    }

    function addPathMenu(){
        var html = '<div class="modal" id="pathMenu" style="position:absolute;left:290px;top:85px;width:127px;height:44px;z-index:99999;display:none">'
            +'<a class="btn btn-large btn-primary ">\u66f4\u591a\u8def\u5f84...</a>'
            +'</div>';
        $('#pathMenu').remove();
        $(document.body).append(html);
    }

    //加载最优路径
    function loadOptimalPath(nodeA,nodeB){
        $.post(topo_path_optimal_path,{srcId:nodeA.id,dstId:nodeB.id,ip:ipp,port:portt},function(dataObj){
            Topo.fadeAllLinks();//淡化所有节点
            Topo.boldLink(dataObj);
        });
    }

    //最优路径显示
    function showOptimalPath(nodeIds){
        Topo.fadeAllLinks();//淡化所有节点
        Topo.boldLink(nodeIds.split(','));
    }

    //显示所有路径
    function showAllPath(nodeA,nodeB){
        $.post(topo_path_optimal_path_all,{srcId:nodeA.id,dstId:nodeB.id},function(dataObj){
            var title = nodeA.id+'\u5230<br/>'+nodeB.id;
            var bodyContent =   '';

            for(var i=0;i<dataObj.length;i++){
                var item = dataObj[i];
                var paths = dataObj[i].path;
                var path = '';
                var nodeLink = '';
                for(var j=0;j<paths.length;j++){
                    path=path+'&#8595;&nbsp;&nbsp;&nbsp;'+paths[j]+'<br/>'
                    nodeLink=nodeLink+paths[j]+',';
                }
                bodyContent = bodyContent+'<tr  style="cursor: default;" id="virtual_item_'+item.nodeId+'">'
                    +'<td onclick="showOptimalPath(\''+nodeLink+'\');">'+path+'</td>'
                    +'<td><a '; //设为最优
                if(item.optimal){//当前已经是最优路径
                    bodyContent = bodyContent+' style="display: none;" ';
                }
                bodyContent = bodyContent
                    +'onclick="window.saveOptimalPath(this,\''+nodeA.id+'\',\''+nodeB.id+'\',\''+nodeLink+'\');return false;" class="setPathBtn btn">\u8bbe\u4e3a\u6700\u4f18</a>'
                    +'</td></tr>';
            }
            //路径  设置
            var bodyAll = '<table class="table table-hover"><thead><tr><th>\u8def\u5f84</th><th>\u8bbe\u7f6e</th></tr></thead>'
                +'<tbody>'
                +bodyContent
                +'</tbody></table>';
            var htm = createMyTopoPopWin(title,bodyAll);
            $('#MyTopoPopWindows').remove();
            $(document.body).append(htm);
            $('#MyTopoPopWindows').width(400);
            $('#MyTopoPopWindows').show();
        });
    }

    //保存最优路径设置
    function saveOptimalPath(btn,srcId,dstId,path){
        $.post(topo_path_optimal_path_save,{srcId:srcId,dstId:dstId,link:path},function(dataObj){
            if(dataObj.status == 'true'){
                $('.setPathBtn').show();
                $(btn).hide();
                alert(dataObj.message);
            }else{
                alert(dataObj.message);
            }
        });
    }

    //动态刷新带宽占用
    function loadRateUsedDataThread(){
       //console.log('loadRateUsedDataThread.....');
        setTimeout("loadRateUsed();loadRateUsedDataThread();",5000);
    }

    //带宽占用详情
    function loadRateDetail(event){

        var leftGap = 25;
        var topGap = 15;
        var e = event.target;

        rate_curr_link = e;
        rate_src_node =  e.nodeA;
        rate_dst_node =  e.nodeZ;

        console.log('loadRateDetail:'+ rate_src_node);

        var div = $("#detailDiv");
        var title = '\u670d\u52a1\u5668\u8be6\u60c5';
        if(!div.attr('id')){
            var html = createDetailWin('detailDiv',title,'',event);
            $(document.body).append(html);
        }
        $('#detailDiv h4').html(title);


        $('#detailDiv').css({
            top: event.pageY+topGap,
            left: event.pageX+260+leftGap,
            width:360
        });
        $('#fabric_content').	hide();
        $('#detailDiv').show();
        refreshRateDetail();
    }

    /**
     * 刷新当前连线带宽占用频率
     */
    var rate_curr_link;
    var rate_src_node;
    var rate_dst_node;
    function refreshRateDetail(){
        $.get(topo_path_detail_rate+'?ip='+ipp+'&port='+portt+'&srcNodeId='+ rate_src_node.id+'&dstNodeId='+rate_dst_node.id,function(dataObj){
            console.log('refresh rate detail div.........');

            var detailContent = '<table  width="100%" id="rateDetailTable">'
                +'<tr><td width="60">\u5f00\u59cb:</td><td colspan="2" align="left">'+ rate_src_node.id+'('+rate_curr_link.portZ+')</td></tr>'; //起始节点
            if(rate_dst_node.type == 'host'){
                detailContent = detailContent +'<tr><td>\u7ed3\u675f:</td><td colspan="2" align="left">'+ rate_dst_node.id+'</td></tr>' //结束节点
            }else{
                detailContent = detailContent +'<tr><td>\u7ed3\u675f:</td><td colspan="2" align="left">'+ rate_dst_node.id+'('+rate_curr_link.portA+')</td></tr>' //结束节点
            }

            detailContent = detailContent  +'<tr><td colspan="3" style="padding-top: 15px;"><div class="progress progress-striped"><div class="bar" style="width: '+dataObj.usedPercent+'%;"></div></div></td></tr>'
                +'<tr><td colspan="3"><div class="progress progress-striped"><div class="bar" style="width: '+dataObj.usedPercentRec+'%;"></div></div></td></tr>'
                +'<tr><td colspan="3"><table class="table table-condensed">'
                +'<thead><tr><td></td><td>transmit</td><td>receive</td></tr></thead><tbody>' //传输率
                +'<tr><td>'+dataObj.usedRateCN+':</td><td>'+ dataObj.usedRate+'</td><td>'+ dataObj.usedRateRec+'</td></tr>' //传输率
                +'<tr><td>'+dataObj.usedBpsCN+':</td><td>'+ dataObj.usedBps+'</td><td>'+ dataObj.usedBpsRec+'</td></tr>'
                +'<tr><td>'+dataObj.usedStatusCN+':</td><td>'+ dataObj.usedStatus+'</td><td>'+ dataObj.usedStatusRec+'</td></tr>'
                +'<tr><td colspan="3" ><input onclick="loadFabric();" id="fabric_button" style="float: right" type="button" class="btn" value="fabric"/></td></tr>'
                +'</tbody></table>';

            var contentTable = '<li id="rateDetailLi"><table><tr><td id="rateDetailDiv">'+detailContent+'</td></tr>'
                +'</table></td><td id="fabric_content" valign="top" style="border: 1px solid #ddd;border-width: 0px 0px 0px 1px;display: none;">'
                +'<div style="height:300px;width:350px;overflow: auto;"><table   class="table table-condensed">'
                +'<thead><tr><td>\u5bf9\u5e94\u8282\u70b9</td><td>\u5e26\u5bbd</td><td>\u64cd\u4f5c</td></tr></thead>'
                +'<tbody id="fabric_body">'
                +'</tbody>'
                +'</table></div></td></tr></table>'
                +'</li>';

            var div = $("#detailDiv");
            var title = '\u5e26\u5bbd\u72b6\u6001';//带宽状态

            if(!div.attr('id')){//未创建
                var html = createDetailWin('detailDiv',title,contentTable);
                $(document.body).append(html);
            }else{//已经创建
                if( $("#rateDetailLi").is(":visible") ){//已经显示
                    $('#rateDetailDiv').html(detailContent);
                }else{
                    $('#detailDiv ul li').remove();
                    $('#detailDiv ul').append(contentTable);
                }
            }
            $('#detailDiv h4').html(title);


            //刷新当前连接线颜色
            var rate1 = parseInt(dataObj.usedRate);
            var rate2 = parseInt(dataObj.usedRateRec);
            //changeLinkColor(rate_src_node.id,rate_dst_node.id,rate1>rate2?rate1:rate2);


        });
    }


    //显示占用带宽最多的5条通过此link的fabric
    function loadFabric(){
        $.get(topo_path_fabric+'?srcNodeId='+ rate_src_node.id+'&dstNodeId='+rate_dst_node.id,function(dataObj){
            var html = '';
            for(var i=0;i<dataObj.length;i++){
                var data = dataObj[i];
                html=html+'<tr><td style="width:30px;">src:'+data.srcId+'('+data.srcPort+')<br>dst:'+ data.dstId+'('+data.dstPort+')</td>' +
                    '<td>'+ data.useRate+'</td>' +
                    '<td><a href="#" class="btn" onclick="changeFabric(\''+data.srcId+'\',\''+data.srcPort+'\',\''+data.dstId+'\',\''+data.dstPort+'\');">\u8c03\u6574</a></td></tr>';
            }
            $('#fabric_body').html(html);
            $('#detailDiv').css({
                width:660
            });
            $('#fabric_content').show();
            return false;
        });
    }

    function changeFabric(srcId,srcPort,dstId,dstPort){
        $.get(topo_path_fabric_set+'?srcId='+ srcId+'&srcPort='+srcPort+'&dstId='+dstId+'&dstPort='+dstPort,function(dataObj){
            alert(dataObj.message);
            $('#detailDiv').hide();
        });
    }


    //带宽占用监控
    function loadRateUsed(){
        //定时刷新带宽占用细节
        if($('#rateDetailTable').is(":visible") && $("#detailDiv").is(":visible")){
            refreshRateDetail();
        }

        //如果开启监控，则定时刷新
         if($('#rateUsedMonitor').val() == 1){
             refreshLinkColor();
         }
        //refreshLinkColor();
    }

    //改变连线颜色
    function changeLinkColor(srcNodeId,dstNodeId,status){
        var transmitIndex = 0;
        var receiveIndex = 0;
        for(var j=0;j<rate_used.length;j++){
            if(status==rate_used[j]){
                transmitIndex = j;
            }
            /*
            if(statusRec==rate_used[j]){
                receiveIndex = j;
            }*/
        }

        //开启监控
        Topo.showLinkMonitor(srcNodeId, dstNodeId, link_colors[transmitIndex]);
        //console.log('src:'+srcNodeId+',des:'+dstNodeId)
        //Topo.changeLinkColor(srcNodeId, dstNodeId, link_colors[transmitIndex]);
    }

    //整体刷新线条颜色
    function refreshLinkColor(){
        $.ajax({ url: topo_path_rate_used+"?ip="+ipp+"&port="+portt, type: "POST", success: function(data){
            //加载到数据...动态刷新
            for(var i=0;i<data.length;i++){
                var srcNodeId = data[i].srcNodeId;
                var dstNodeId = data[i].dstNodeId;
                var status = parseInt(data[i].status);
               // var statusRec = parseInt(data[i].statusRec);

                changeLinkColor(srcNodeId,dstNodeId,status);
            }
        }});
    }
 
    //Topo全屏
    var isFullNow = false;
    function fullTopoInWin(){
        if(!isFullNow){//全屏

            fullscreen();

            $('#logobar').hide();
            $('#sidebar').hide();
            $('#middle-top').hide();
            $('#middle-left').hide();
            $('#headbar').hide();
            $('#wrapbar').css("margin-left","0px");
            $('#sizediv').css({
                width:'100%'
            });
            $('#sizediv').css({
                height:'100%'
            });
            $('#sizediv').css({
                top:'0',
                left:'0'
            });
            $('#middle-right').css({
                width:'90%'
            });
            $("#canvas_circle").css({height: '623px'});	
            $("#canvas_circle").css({width: '1122px'});
            $("#canvas-bg").css({height: '700px'});
            $("#chart").css({height: '650px'});
            isFullNow = true;
            $('#resizeBtn').attr('src',prefix+'img/rest.png');
            $('#resizeBtn').attr('title','\u6062\u590d\u539f\u72b6');//恢复原状


        }else{
            exitFullscreen();

            
            $('#right-top').css({
                height:'600px'
            });
            $('#logobar').show();
            $('#sidebar').show();
            $('#middle-top').show();
            $('#middle-left').show();
            $('#headbar').show();
            $('#wrapbar').css("margin-left","185px");
            $('#middle-right').css({
                width:'48.71795%'
            });
            $("#canvas-bg").css({height: '570px'});
            $("#chart").css({height: '500px'});
            $("#canvas_circle").css({height: '450px'});
            $("#canvas_circle").css({width: '494px'});
            isFullNow = false;
            $('#resizeBtn').attr('src',prefix+'img/max_size.png');
            $('#resizeBtn').attr('title','\u70b9\u51fb\u5168\u5c4f'); //点击全屏


        }

        resizeCanvas();
    }


    var fullscreen=function(){
        elem=document.body;
        if(elem.webkitRequestFullScreen){
            elem.webkitRequestFullScreen();
        }else if(elem.mozRequestFullScreen){
            elem.mozRequestFullScreen();
        }else if(elem.requestFullScreen){
            elem.requestFullscreen();
        }else if(elem.msRequestFullscreen) {
            elem.msRequestFullscreen();
        }
        else{
            //浏览器不支持全屏API或已被禁用
            alert('\u5f53\u524d\u6d4f\u89c8\u5668\u4e0d\u652f\u6301\u8bf7\u6309F11\u5b8c\u6210\u6b64\u64cd\u4f5c');
        }
    }
    var exitFullscreen=function(){
        var elem=document;
        if(elem.webkitExitFullscreen){
            elem.webkitExitFullscreen();
        }else if(elem.mozCancelFullScreen){
            elem.mozCancelFullScreen();
        }else if(elem.exitFullscreen){
            elem.exitFullscreen();
        }else{
            //浏览器不支持全屏API或已被禁用
            alert('\u5f53\u524d\u6d4f\u89c8\u5668\u4e0d\u652f\u6301\u8bf7\u6309F11\u5b8c\u6210\u6b64\u64cd\u4f5c');
        }
    }

    exports.initAll = initAll;
    exports.showDetail = showDetail;
    exports.loadRateUsed = loadRateUsed;
    exports.loadRateUsedDataThread = loadRateUsedDataThread;
    exports.loadVirtualNetItems = loadVirtualNetItems;
    exports.delVirtualNetItem = delVirtualNetItem;
    exports.updateInterval = updateInterval;
    exports.saveVirtualNet = saveVirtualNet;
    exports.createVirtualNet = createVirtualNet;
    exports.saveOptimalPath = saveOptimalPath;
    exports.centerNode = centerNode;
    exports.getCanvasDivHtml = getCanvasDivHtml;
    exports.getToolBarHtml = getToolBarHtml;
    exports.fullTopoInWin = fullTopoInWin;
    exports.showVirtualNetAllNode = showVirtualNetAllNode;
    exports.link_colors = link_colors;
    exports.base_path = base_path;
    exports.showOptimalPath = showOptimalPath;
    exports.loadFabric = loadFabric;
    exports.changeFabric = changeFabric;
})($, window);

String.prototype.trim = function() {
    return this.replace(/^\s\s*/, '').replace(/\s\s*$/, '');
}

$(document).ready(function(){



    //主拓扑，右上
    var toolBarHtml = getToolBarHtml();
    var canvasHtml = getCanvasDivHtml();
    var html = '<div id="canvasPanelDiv" class="dash">';
    html = html+toolBarHtml+canvasHtml+'</div>';
    $('#chart').html(html);

    //屏蔽系统右键菜单，否则会遮挡自定义右键菜单
    $(document).ready(function(){
        $(document).bind("contextmenu",function(e){
            return false;
        });
    });
    

});