var uploadBars = [];

$(document).ready(function() {
  var dropZone = document.getElementById('upload-list');
  dropZone.addEventListener('dragover', handleDragOver, false);
  dropZone.addEventListener('dragenter', handleDragEnter, false);
  dropZone.addEventListener('drop', handleFileSelect, false);
  console.log("fileupload");

  $('#upload-button').click(function(e) {
    console.log( "#uploadDICOM" );
    uploadStart();
  });
});

function handleDragOver(evt) {
	evt.stopPropagation();
	evt.preventDefault();
}
function handleDragEnter(evt) {
	evt.stopPropagation();
	evt.preventDefault();
}

function handleFileSelect(e) {
  e.stopPropagation();
  e.preventDefault();
  console.log("handleFileSelect");

  uploadBars = [];
  $("#upload-list").html('<li data-role="list-divider" role="heading" class="ui-li ui-li-divider ui-bar-b ui-corner-top ui-corner-bottom">Drag and drop DICOM files here</li>');

  var items = e.dataTransfer.items;
  for (var i=0; i<items.length; i++) {
    // webkitGetAsEntry is where the magic happens
    var item = items[i].webkitGetAsEntry();
    if (item) {
		recurseDirectory(item,item.name);
    }
  }
}

function recurseDirectory(dirEntry, path) {
	var dirReader = dirEntry.createReader();
    function helper() {
        dirReader.readEntries(function(entries) {
			if(entries.length) {
				entries.forEach(function(entry) {
					if (entry.isDirectory) {
						recurseDirectory(entry,entry.name);
					}else if (entry.isFile) {
						entry.file(function(fd) {
							console.log("File:", fd.name+ "/"+path);
							var bar = new StatusBar($("#upload-list"),fd,path);
							uploadBars.push({"file":fd,"path":path,"status":bar});
						});
					}
				});
				helper();
			}
        });
    }
    helper();
}

// On upload click
function uploadStart() {
	console.log( "uploadStart" );
	//new study first
	for (var i=0; i<uploadBars.length; i++) {
		if(uploadBars[i].status.done == false) uploadInstance(uploadBars[i].file,uploadBars[i].status,uploadBars[i].path);
	}
}

function uploadInstance(file,status,path)
{
	function resolve(data){
            //console.log( data );
            status.setProgress(100);
			updateTotalProgress();
        }
	function reject( jqXHR,textStatus,errorThrown ){
			//console.log( jqXHR,textStatus,errorThrown );
			status.setProgress(0);
		}
	function progress(percent){
            status.setProgress(percent);
        }

    //console.log(instance);
    var fd = new FormData();
    fd.append('file', file);
    PostData("/instances/"+file.name+"/of/"+path+"/", fd, resolve,reject,progress);
}

function updateTotalProgress()
{
	var count = 0;
	for (var i=0; i<uploadBars.length; i++) {
		if(uploadBars[i].status.done == true) count += 1;;
	}

	var progress = parseInt(count / uploadBars.length * 100, 10);
	$('#progress .label').text('Uploading: ' + progress + '%');
	$('#progress .bar')
        .css('width', progress + '%')
        .css('background-color', 'green');
}



///////////////////////////////////////////////////////////////////////////////
function StatusBar(parent,fd,path)
{
	this.done = false;
    this.bar = $('<li class="pending-file ui-li ui-li-static ui-body-c"></li>');
    this.filename = $("<div class='filename'></div>").appendTo(this.bar);
	this.filename.html(path+"/"+fd.name.shortStr(30));
    this.progress = $("<div class='progressBar'><div>&nbsp;</div></div>").appendTo(this.bar);

    parent.append(this.bar);

    this.setProgress = function(progress)
    {       
        var progressBarWidth =progress*this.progress.width()/ 100;  
        this.progress.find('div').animate({ width: progressBarWidth }, 10).html(progress + "% ");
		if(progress==100) {this.bar.hide(); this.done = true;}
    }
}

function PostData(uploadURL,formData, resolve,reject,progress)
{
    var jqXHR=$.ajax({
            xhr: function() {
            var xhrobj = $.ajaxSettings.xhr();
            if (xhrobj.upload) {
                    xhrobj.upload.addEventListener('progress', function(event) {
                        var percent = 0;
                        var position = event.loaded || event.position;
                        var total = event.total;
                        if (event.lengthComputable) {
                            percent = Math.ceil(position / total * 100);
                        }
                        //Set progress
                        progress(percent);
                    }, false);
                }
            return xhrobj;
        },
    url: uploadURL,
    type: "POST",
    contentType:false,
    processData: false,
        cache: false,
        data: formData,
        success: resolve,
		error: reject
    });
}

String.prototype.shortStr = function(num){
  var left = this.slice(0, num+1).replace(/\s+/g,' ').replace(/\s\S*$/,'');
  if(this != left) left += '...'
  return left;
}