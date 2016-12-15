function showChart(nodes,links){
    var myChart=echarts.init(document.getElementById('chart'));

    option = {
        tooltip : {
            trigger: 'item'
        },
        toolbox: {
            show : false,
            feature : {
                restore : {show: true},
                saveAsImage : {show: true}
            }
        },
        legend: {
            x: 'left',
            selectedMode: false,
            data:['switch','host']
        },
        series : [
            {
                type:'force',
                name : "关系",
                categories : [
                    {
                        name: 'switch'
                    },
                    {
                        name:'host'
                    }
                ],
                itemStyle: {
                    normal: {
                        label: {
                            show: true,
                            textStyle: {
                                color: '#333'
                            }
                        },
                        nodeStyle : {
                            brushType : 'both',
                            strokeColor : 'rgba(255,215,0,0.4)',
                            lineWidth : 1
                        }
                    },
                    emphasis: {
                        label: {
                            show: false
                            // textStyle: null      // 默认使用全局文本样式，详见TEXTSTYLE
                        },
                        nodeStyle : {
                            //r: 30
                        },
                        linkStyle : {}
                    }
                },
                useWorker: false,
                minRadius : 15,
                maxRadius : 25,
                gravity: 100,
                scaling: 10,
                linkSymbol: 'none',



                nodes:nodes,
                links:links
            }
        ]
    };
    myChart.setOption(option);
}