function getOutputsToggleButton(category){
    return category.querySelector(".categoryHeader button")
}

function changeVisibility(category, show){
        body = category.querySelector(".categoryBody")
        typeSelector = category.querySelector(".categoryHeader select")
        header = category.querySelector(".categoryHeader")
        
        btn = getOutputsToggleButton(category)
        i = btn.querySelector("i")
        if( body && typeSelector && i){
            if( show){
                i.classList.remove("fa-arrow-down")
                i.classList.add("fa-arrow-up")
                body.classList.remove("hidden")
            }else{
                i.classList.remove("fa-arrow-up")
                i.classList.add("fa-arrow-down")
                body.classList.add("hidden")
            }
            if( btn)
                if( typeSelector.value == -1)
                    btn.classList.add("hidden")
                else
                    btn.classList.remove("hidden")
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
    function getValue(cat,name){
        var inp = cat.querySelector("input[name='" + name + "']")
        if( inp)
            return inp.value
        return undefined
    }
    function addConfiguration(json, cat){
        hostname = getValue(cat, 'hostname')
        sslcert = getValue(cat, 'sslcert')
        mqtturl = getValue(cat, 'mqtturl')
        username = getValue(cat, 'username')
        password = getValue(cat, 'password')
        authenticationMethod= Number(getValue(cat, 'authenticationMethod'))
        hour = getValue(cat, 'hour')
        minute = getValue(cat, 'minute')
        second = getValue(cat, 'second')
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
        var typeField = cat.querySelector("select")
        var p = typeField.name.split("_")
        json.outputs.push({ 
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
             var inputFields = inp.querySelectorAll("input")
             var divider = 1000
             var inputPort = 0
             var outputPort = 0
             var mqttname =""
             inputFields.forEach( fld=>{
                var p = fld.name.split("_")
                if(p.length == 3){
                    inputPort = Number(p[2])
                    outputPort = Number(p[1])
                    if( fld.name.startsWith("divider") && fld.value != undefined)
                        divider = Number(fld.value)
                    else
                        mqttname = fld.value
                }
             });
             if(mqttname !="")
                json.counters.push({ 
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
function categoryHeaderUpdateActive(name){
    var el = document.getElementsByName(name)[0]
    // Need to update parentNode, but parentNode property of element is readonly
    var categories = document.querySelectorAll(".category");
    categories.forEach((cat) => {
        var catHdr = cat.querySelector(".categoryHeader")
        var typeSelect = catHdr.querySelector("select")
        var toggleButton = catHdr.querySelector("button")
          
        if( typeSelect && toggleButton && typeSelect.name == name){
             if( typeSelect.value != -1){
                catHdr.classList.remove("inactive")
                toggleButton.classList.remove("hidden")
            }
            else{
                catHdr.classList.add("inactive")
                changeVisibility(cat,false)
            }
                
        }
    })
}
function copyOutputsHtml(json){
    var outputTemplate = document.querySelector("[id='outputTemplate']")
    var inputTemplate = document.querySelector("[id='inputTemplate']")
    for( var oidx=8; oidx >=0; oidx--){
        var output = json.outputs.find(out=>out.port == oidx)
        var newOutput = outputTemplate.cloneNode(true);
        if( output )
            newOutput.querySelector("select").value = output.type
        newOutput.id="output_" + oidx;
        if(newOutput.querySelector("select").value == -1 ){
            newOutput.querySelector(".categoryHeader").classList.add('inactive')
            newOutput.querySelector(".categoryHeader button").classList.add('hidden')
        }
            
        newOutput.querySelector("select").name = "type_" + oidx;
        newOutput.querySelector("span").innerHTML = "Output Pin " + oidx;
        getOutputsToggleButton(newOutput).id = "out_" + oidx;
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
function setFieldValue(field, value, fieldType="input"){
    var field= document.querySelector(fieldType +"[name='" + field + "']")
    if( field)
        field.value = value
    else
        console.log("Field " + fieldType  + ":" + field + " not found")
}
function buildConfigHtml(json){
    json.network.sslcert
        if( json.network != undefined){
            setFieldValue( 'hostname',json.network.hostname );
            setFieldValue( 'sslcert',json.network.sslcert );
        }
        if( json.mqtt != undefined){   
            setFieldValue( 'mqtturl',json.mqtt.mqtturl );
            setFieldValue( 'username',json.mqtt.username );
            setFieldValue( 'password',json.mqtt.password );
            setFieldValue( 'authenticationMethod',json.mqtt.authenticationMethod, "select" );
        }
        if( json.schedule != undefined){
            setFieldValue( 'hour',json.schedule.hour );
            setFieldValue( 'minute',json.schedule.minute );
            setFieldValue( 'second',json.schedule.second );
        }
        copyOutputsHtml(json)
        if( json.outputs != undefined){
            json.outputs.forEach(output=>{
                setFieldValue('type_' + output.port,output.type, "select")
            })
        }

}
document.addEventListener("DOMContentLoaded", (event) => {
  getJSON("/api/config",(err,result)=>{
    if(err == null)
        buildConfigHtml(result)
  })
});