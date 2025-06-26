 // Update


document.addEventListener("DOMContentLoaded", (event) => {

    document.getElementById('uploadButton').addEventListener('click', function(e) {

        document.getElementById("uploadButton").disabled = true;

        var file = document.getElementById("updateFile").files[0];
        var fileSize = file.size;
        var chunkSize = Math.ceil(file.size / 100, 100);
        var chunk = 0;

        console.log('filesize ', fileSize);
        console.log('chunksize: ', chunkSize);
        document.getElementById("progress-bar").classList.remove("hidden")
        sendFileSlice(file, chunkSize, chunk, 100);
    });
    document.getElementById("updateFile").removeEventListener('change',enableDisableUploadButton);
    document.getElementById("updateFile").addEventListener('change',enableDisableUploadButton );

});
function enableDisableUploadButton(e){
            document.getElementById("uploadButton").disabled = ( document.getElementById("updateFile").files.size  == 0);
}
    function sendFileSlice(file, chunkSize, currentChunk, numChunks) {
        // console.log("Sending Chunk " + currentChunk + " / " + numChunks);

        if (currentChunk <= numChunks - 1) {
            var offset = currentChunk * chunkSize;
            var xhttp = new XMLHttpRequest();

            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    console.log("Chunk " + currentChunk + " sent");
                    currentChunk++;

                    var percentComplete = Math.round(currentChunk * chunkSize * 100 / file.size);
                    if (percentComplete > 100) {
                        percentComplete = 100;
                    }

                    document.getElementById('progress-bar').innerHTML = percentComplete.toString() + '%';
                    document.getElementById("progress-bar").style.width = percentComplete.toString() + '%'

                    setTimeout(() => {
                        sendFileSlice(file, chunkSize, currentChunk, numChunks); 
                    }, 10);
                }
                // else
                // {
                //     console.log(this.readyState, this.status);
                //     alert("Upload failed!");
                //     document.getElementById("uploadButton").disabled = false;
                // }
            };
            xhttp.open("POST", "/api/update", true);
            xhttp.setRequestHeader("X-OTA-SIZE", file.size);
            xhttp.send(file.slice(offset, offset + chunkSize));
        }
        else
        {
            document.getElementById("progress-bar").classList.add("hidden")
            document.getElementById("uploadButton").disabled = true;
            alert("Upload Done, rebooting...");
        }
    }
