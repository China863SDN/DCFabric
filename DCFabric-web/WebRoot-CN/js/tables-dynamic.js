
    //jQuery DataTables part

    /* Set the defaults for DataTables initialisation */
    $.extend( true, $.fn.dataTable.defaults, {
        "sDom": "<'row-fluid'<'span6'l><'span6'f>r>t<'row-fluid'<'span6'i><'span6'p>>",
        "sPaginationType": "bootstrap",
        "oLanguage": {
            "sLengthMenu": "_MENU_ records per page"
        }
    } );


    /* Default class modification */
    $.extend( $.fn.dataTableExt.oStdClasses, {
        "sWrapper": "dataTables_wrapper form-inline"
    } );


    /* API method to get paging information */
    $.fn.dataTableExt.oApi.fnPagingInfo = function ( oSettings )
    {
        return {
            "iStart":         oSettings._iDisplayStart,
            "iEnd":           oSettings.fnDisplayEnd(),
            "iLength":        oSettings._iDisplayLength,
            "iTotal":         oSettings.fnRecordsTotal(),
            "iFilteredTotal": oSettings.fnRecordsDisplay(),
            "iPage":          oSettings._iDisplayLength === -1 ?
                0 : Math.ceil( oSettings._iDisplayStart / oSettings._iDisplayLength ),
            "iTotalPages":    oSettings._iDisplayLength === -1 ?
                0 : Math.ceil( oSettings.fnRecordsDisplay() / oSettings._iDisplayLength )
        };
    };


    /* Bootstrap style pagination control */
    $.extend( $.fn.dataTableExt.oPagination, {
        "bootstrap": {
            "fnInit": function( oSettings, nPaging, fnDraw ) {
                var oLang = oSettings.oLanguage.oPaginate;
                var fnClickHandler = function ( e ) {
                    e.preventDefault();
                    if ( oSettings.oApi._fnPageChange(oSettings, e.data.action) ) {
                        fnDraw( oSettings );
                    }
                };

                $(nPaging).addClass('pagination').append(
                    '<ul>'+
                        '<li class="prev disabled"><a href="#">'+oLang.sPrevious+'</a></li>'+
                        '<li class="next disabled"><a href="#">'+oLang.sNext+'</a></li>'+
                        '</ul>'
                );
                var els = $('a', nPaging);
                $(els[0]).bind( 'click.DT', { action: "previous" }, fnClickHandler );
                $(els[1]).bind( 'click.DT', { action: "next" }, fnClickHandler );
            },

            "fnUpdate": function ( oSettings, fnDraw ) {
                var iListLength = 5;
                var oPaging = oSettings.oInstance.fnPagingInfo();
                var an = oSettings.aanFeatures.p;
                var i, ien, j, sClass, iStart, iEnd, iHalf=Math.floor(iListLength/2);

                if ( oPaging.iTotalPages < iListLength) {
                    iStart = 1;
                    iEnd = oPaging.iTotalPages;
                }
                else if ( oPaging.iPage <= iHalf ) {
                    iStart = 1;
                    iEnd = iListLength;
                } else if ( oPaging.iPage >= (oPaging.iTotalPages-iHalf) ) {
                    iStart = oPaging.iTotalPages - iListLength + 1;
                    iEnd = oPaging.iTotalPages;
                } else {
                    iStart = oPaging.iPage - iHalf + 1;
                    iEnd = iStart + iListLength - 1;
                }

                for ( i=0, ien=an.length ; i<ien ; i++ ) {
                    // Remove the middle elements
                    $('li:gt(0)', an[i]).filter(':not(:last)').remove();

                    // Add the new list items and their event handlers
                    for ( j=iStart ; j<=iEnd ; j++ ) {
                        sClass = (j==oPaging.iPage+1) ? 'class="active"' : '';
                        $('<li '+sClass+'><a href="#">'+j+'</a></li>')
                            .insertBefore( $('li:last', an[i])[0] )
                            .bind('click', function (e) {
                                e.preventDefault();
                                oSettings._iDisplayStart = (parseInt($('a', this).text(),10)-1) * oPaging.iLength;
                                fnDraw( oSettings );
                            } );
                    }

                    // Add / remove disabled classes from the static elements
                    if ( oPaging.iPage === 0 ) {
                        $('li:first', an[i]).addClass('disabled');
                    } else {
                        $('li:first', an[i]).removeClass('disabled');
                    }

                    if ( oPaging.iPage === oPaging.iTotalPages-1 || oPaging.iTotalPages === 0 ) {
                        $('li:last', an[i]).addClass('disabled');
                    } else {
                        $('li:last', an[i]).removeClass('disabled');
                    }
                }
            }
        }
    } );

    var unsortableColumns = [];
    var oTables;
    $('#datatable-table').find('thead th').each(function(){
        if ($(this).hasClass( 'no-sort')){
            unsortableColumns.push({"bSortable": false});
        } else {
            unsortableColumns.push(null);
        }
    });

    function showTable(data) {
        if (oTables) {
            oTables.fnDestroy();
        }
        oTables = $('#datatable-table').dataTable({
            "sDom": "<'row-fluid table-top-control'<'span6 hidden-phone per-page-selector'l><'span6'f>r>t<'row-fluid table-bottom-control'<'span6'i><'span6'p>>",
            "oLanguage": {
                "sProcessing": "正在加载中...",
                "sLengthMenu": "每页显示_MENU_条记录",
                "sEmptyTable": "表中无数据存在",
                "sZeroRecords": "对不起，查询不到相关数据！",
                "sInfo": "当前显示_START_到_END_条，共_MAX_条记录",
                "sInfoFiltered": "数据表中共为_MAX_条记录",
                "sSearch": "搜索",
                "oPaginate": {
                    "sFirst": "首页",
                    "sPrevious": "上一页",
                    "sNext": "下一页",
                    "sLast": "末页"
                }
            },
            "aoColumns": unsortableColumns,
            "aaData": data
        });

    }
    var unsortableColumns1 = [];
    var oTables1;
    $('#datatable-table1').find('thead th').each(function(){
        if ($(this).hasClass( 'no-sort')){
            unsortableColumns1.push({"bSortable": false});
        } else {
            unsortableColumns1.push(null);
        }
    });

    function showTables(data,total) {
        if (oTables1) {
            oTables1.fnDestroy();
        }
        oTables1 = $('#datatable-table1').dataTable({
            "sDom": "<'row-fluid table-top-control'<'span6 hidden-phone per-page-selector'l><'span6'f>r>t<'row-fluid table-bottom-control'<'span6'i><'span6'p>>",
            "oLanguage": {
                "sProcessing": "正在加载中...",
                "sLengthMenu": "每页显示_MENU_条记录",
                "sEmptyTable": "表中无数据存在",
                "sZeroRecords": "对不起，查询不到相关数据！",
                "sInfo": "当前显示_START_到_END_条，共"+total+"条记录",
                "sInfoFiltered": "数据表中共为_MAX_条记录",
                "sSearch": "搜索",
                "oPaginate": {
                    "sFirst": "首页",
                    "sPrevious": "上一页",
                    "sNext": "下一页",
                    "sLast": "末页"
                }
            },
            "aoColumns": unsortableColumns1,
            "aaData": data
        });

    }
    