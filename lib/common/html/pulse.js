function changeVisibility(category, show){
        body = category.querySelector(".categoryBody")
        btn = category.querySelector(".categoryHeader button")
        i = btn.querySelector("i")
        if( show){
            i.classList.remove("fa-arrow-down")
            i.classList.add("fa-arrow-up")
            body.classList.remove("hidden")
        }else{
            i.classList.remove("fa-arrow-up")
            i.classList.add("fa-arrow-down")
            body.classList.add("hidden")
        }

    }
    function toggle(thebutton){
        categories = document.querySelectorAll(".category");
        categories.forEach((cat) => {
             body = cat.querySelector(".categoryBody")
             hidden = body.classList.contains("hidden");
             btn = cat.querySelector(".categoryHeader button")
             if( btn.id == thebutton)
                changeVisibility(cat,hidden)
             else
                changeVisibility(cat, false);
        });
    }
    function addConfiguration(json, cat){
        hostname = cat.querySelector("input[name='hostname']").value
        sslcert = cat.querySelector("input[name='sslcert']").value
        mqtturl = cat.querySelector("input[name='mqtturl']").value
        username = cat.querySelector("input[name='username']").value
        password = cat.querySelector("input[name='password']").value
        authenticationMethod= cat.querySelector("input[name='authenticationMethod']").value
        hour = cat.querySelector("input[name='hour']").value
        minute = cat.querySelector("input[name='minute']").value
        second = cat.querySelector("input[name='second']").value
        json.network ={
            hostname: hostname,
            sslcert: sslcert
        }
        json.mqtt ={
            mqtturl: mqtturl,
            username: username,
            password: password,
            authenticationMethod: authenticationMethod
        }
        json.schedule ={
            hour: hour,
            minute: minute,
            second: second
        }
    }
    function addOutput(json, cat){
        typeField = cat.querySelector("select")
        p = typeField.name.split("_")

        json.outputs.add({ 
            type: typeField.value,
            port: Number(p[1])
        })
    }
    function sendConfig(){
        json={
            counters: [],
            outputs:[]
        };
        categories = document.querySelectorAll(".category");
            categories.forEach((cat) => {
                if(cat.querySelector("button[id='configuration']"))
                    addConfiguration(json, cat)
                else
                    addOutput(json, cat)  
            });
        inputs = document.querySelectorAll(".input");
        inputs.forEach((inp)=>{
             inputFields = cat.querySelector("input")
             divider = 1000
             inputPort = 0
             outputPort = 0
             mqttname =""
             inputFields.forEach( fld=>{
                p = fld.name.split("_")
                inputPort = Number(p[2])
                outputPort = Number(p[1])
                if( fld.name.startsWith("divider") && fld.value != undefined)
                    divider = fld.value
                else
                    mqttname = fld.value

             });
             if(mqttname !="")
                json.counters.add({ 
                    inputPort: inputPort,
                    outputPort: outputPort,
                    divider: divider,
                    mqttname: mqttname
                })
        }) 
        var xhr = new XMLHttpRequest();
        var url = "/api/config";
        xhr.open("POST", url, true);
        xhr.setRequestHeader("Content-Type", "application/json");
        var data = JSON.stringify(json);
        xhr.send(data);         
    }
    function getJSON(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function() {
      var status = xhr.status;
      if (status === 200) {
        callback(null, xhr.response);
      } else {
        callback(status, xhr.response);
      }
    };
    xhr.send();
};
function copyOutputsHtml(json){
    var outputTemplate = document.querySelector("[id='outputTemplate']")
    var inputTemplate = document.querySelector("[id='inputTemplate']")
    for( var oidx=8; oidx >=0; oidx--){
        var output = json.outputs.find(out=>out.port == oidx)
        var newOutput = outputTemplate.cloneNode(true);
        if( output )
            newOutput.querySelector("select").value = output.type
        if(newOutput.querySelector("select").value == -1 )
            newOutput.querySelector(".categoryHeader").classList.add('inactive')
        newOutput.querySelector("select").name = "type_" + oidx;
        newOutput.querySelector("span").innerHTML = "Output Pin " + oidx;
        var newInputTemplate = newOutput.querySelector("[id='inputTemplate']")
        for( var iidx=15; iidx >=0; iidx--){
            var newInput = inputTemplate.cloneNode(true)
            var header = newInput.querySelector("div[class='inputLabel']")
            if( header)
                header.innerHTML = "Input Port " + iidx 
            var input = json.counters.find(inp=>inp.inputPort == iidx && inp.outputPort == oidx);
            newInput.querySelectorAll( "input").forEach((inp)=>{
                var p = inp.name.split("_");
                inp.name= p[0] + "_" + oidx + "_" + iidx;
                if(input)
                    if(p[0] == "divider"){
                        if(input.divider)
                            inp.value = input.divider                    
                    }
                    else
                        inp.value = input.mqttname
            })  
            newInputTemplate.after(newInput)
        }
        newInputTemplate.remove()
        outputTemplate.after(newOutput)
    }
    outputTemplate.remove()

}
function buildConfigHtml(json){
    json.network.sslcert
        if( json.network != undefined){
            document.querySelector("input[name='hostname']").value = json.network.hostname
            document.querySelector("input[name='sslcert']").value = json.network.sslcert
        }
        if( json.mqtt != undefined){   
            document.querySelector("input[name='mqtturl']").value = json.mqtt.mqtturl
            document.querySelector("input[name='username']").value = json.mqtt.username
            document.querySelector("input[name='password']").value = json.mqtt.password
            document.querySelector("input[name='authenticationMethod']").value = json.mqtt.authenticationMethod
        }
        if( json.schedule != undefined){
            document.querySelector("input[name='hour']").value = json.schedule.hour
            document.querySelector("input[name='minute']").value = json.schedule.minute
            document.querySelector("input[name='second']").value = json.schedule.second
        }
        copyOutputsHtml(json)
        if( json.outputs != undefined){
            json.outputs.forEach(output=>{
                document.querySelector("select[name='type_" + output.port + "']").value = output.type
            })
        }

}
document.addEventListener("DOMContentLoaded", (event) => {
  getJSON("/api/config",(err,result)=>{
    if(err == null)
        buildConfigHtml(result)
  })
});