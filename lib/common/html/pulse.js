function getOutputsToggleButton(category){
    return category.querySelector(".categoryHeader button")
}

function changeVisibility(category, show){
        body = category.querySelector(".categoryBody")
        typeSelector = category.querySelector(".categoryHeader select")
        header = category.querySelector(".categoryHeader")
        
        btn = getOutputsToggleButton(category)
        i = btn.querySelector("i")
        if( body  && typeSelector && i){
            if( show){
                i.classList.remove("fa-arrow-down")
                i.classList.add("fa-arrow-up")
                body.classList.remove("hidden")
            }else{
                i.classList.remove("fa-arrow-up")
                i.classList.add("fa-arrow-down")
                body.classList.add("hidden")
            }
            if( btn )
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
             if( btn)
                if( btn.id == thebutton)
                    changeVisibility(cat,hidden)
                else
                    changeVisibility(cat, false);
        });
    }
    function getFieldValue(cat,name, type){
        var inp = cat.querySelector(type + "[name='" + name + "']")
        if( inp)
            return inp.value
        return undefined
    }
    function buildTimeArray(schedule, max){
        let rc = [];
        if( schedule == undefined||schedule ==""){
            return rc;
        }
        var s=schedule.replace(" ","")
        if( s == "*")
            for(idx=0; idx< max; idx++)
                rc.push(idx)
        else {
            var p=s.split(",")
            for(idx=0; idx< p.length; idx++){
                var pp=p[idx].split("-");
                if( pp.length == 2)
                    for(iidx= Number(pp[0]); iidx <= Number(pp[1]);iidx++)
                        rc.push(iidx)
                else
                    rc.push(Number(pp[0]));
                }
        }
        return rc;
    }
    function addConfiguration(json, cat){
        hostname = getFieldValue(cat, 'hostname', "input")
        sslhost = getFieldValue(cat, 'sslhost', "input")
        sslhostkey = getFieldValue(cat, 'sslhostkey', "input")
        sslca = getFieldValue(cat, 'sslca', "input")
        logdestination = getFieldValue(cat, 'logdestination', "input")
        
        mqtturl = getFieldValue(cat, 'mqtturl', "input")
        username = getFieldValue(cat, 'username', "input")
        password = getFieldValue(cat, 'password', "input")
        authenticationMethod= Number(getFieldValue(cat, 'authenticationMethod', "select"))
        topic= getFieldValue(cat, 'topic', "input")
        hour = buildTimeArray(getFieldValue(cat, 'hour', "input"),24)
        minute = buildTimeArray(getFieldValue(cat, 'minute', "input"),60)
        second = buildTimeArray(getFieldValue(cat, 'second', "input"),60)
        
        json.network ={
            hostname: hostname,
            sslhost: sslhost,
            sslhostkey: sslhostkey,
            sslca: sslca,
            logdestination: logdestination
        }
        json.mqtt ={
            mqtturl: mqtturl,
            username: username,
            password: password,
            authenticationMethod: authenticationMethod,
            topic: topic
        }
        json.schedule ={
            hour: hour,
            minute: minute,
            second: second
        }
    }
    function addOutput(json, cat){
        var typeField = cat.querySelector("select")
        if( typeField != null){
            var p = typeField.name.split("_")
            json.outputs.push({ 
                type: parseInt( typeField.value ),
                port: Number(p[1])
            })
        }
        else
            json.outputs.push({ 
                type: 0, // EMeter
                port:0xFF
            })
    }
    function buildJsonFromHtml(){
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
             else{
                if( outputPort == 0xFF)
                json.counters.push({ 
                    inputPort: inputPort,
                    outputPort: outputPort,
                    divider: 100,
                    unconfigured: true,
                    mqttname: "unknown_" + inputPort
                })
             }
        }) 
        return json
    }
    function sendConfig(e){
        e.preventDefault(); //this prevents default action
        json = buildJsonFromHtml();
        var xhr = new XMLHttpRequest();
        var url = "/api/config";
        xhr.open("POST", url, true);
        xhr.responseType = 'json';
        xhr.onreadystatechange = function() {
            var status = xhr.status;
            if(e != undefined && xhr.readyState == 4){
                if ( xhr.status == 200) {
                    window.location.reload();
                } else {
                    alert("Error validating MQTT connection")
                }
            }
        };

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
    return false; // prevent immediate submit. Submit happens in callback
};
    function sendResetTicks(e){
        e.preventDefault(); //this prevents default action
        var xhr = new XMLHttpRequest();
        var url = "/api/resetTicks";
        xhr.open("GET", url, true);
        xhr.responseType = 'json';
        xhr.setRequestHeader("Content-Type", "application/json");
        var data = "{ \"resetTicks\": true }";
        xhr.send(data);         
    }


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
function copyInputHtml(outer, json, oidx){
        var inputTemplate = document.querySelector("[id='inputTemplate']")
        for( var iidx=0; iidx < 16; iidx++){
            var newInput = inputTemplate.cloneNode(true)
            newInput.removeAttribute('id');
            var header = newInput.querySelector("div[class='inputLabel']")
            if( header)
                header.innerHTML = "Input Port " + iidx 
            
            if(json && json.counters){
                var input = json.counters.find(inp=>inp.inputPort == iidx && inp.outputPort == oidx);
                var labels = newInput.querySelectorAll("label")
                for (var i=0; i < labels.length; i++) {
                   labels[i].setAttribute("for", labels[i].getAttribute("for") +  "_" + oidx + "_" + iidx);
                }
                newInput.querySelectorAll( "input").forEach((inp)=>{
                    var p = inp.name.split("_");
                    inp.name= p[0] + "_" + oidx + "_" + iidx;
                    inp.id=inp.name;
                    if(input && input.unconfigured == undefined)
                        if(p[0] == "divider"){
                            if(input.divider)
                                inp.value = input.divider                    
                        }
                        else
                            inp.value = input.mqttname
                })
                newInput.querySelectorAll( "div[id='lastCount']").forEach((inp)=>{
                    inp.name= "lastCount_" + oidx + "_" + iidx;
                    inp.id=inp.name;
                })

            }
            outer.appendChild(newInput)
        }

}
function copyOutputsHtml(json){
    var outputTemplate = document.querySelector("div[id='outputTemplate']")
    var formdiv = document.querySelector("form div")
    for( var oidx=0; oidx <8; oidx++){
        var output = null;
        if( json && json.outputs)
            output = json.outputs.find(out=>out.port == oidx)
        var newOutput = outputTemplate.cloneNode(true);
        if( output )
            newOutput.querySelector("select").value = output.type
        newOutput.id="output_" + oidx;
        if(newOutput.querySelector("select").value == -1 ){
            newOutput.querySelector(".categoryHeader").classList.add('inactive')
            newOutput.querySelector(".categoryHeader button").classList.add('hidden')
        }
        newOutput.querySelector("label").setAttribute("for", "type_" + oidx);
        newOutput.querySelector("select").name = "type_" + oidx;
        newOutput.querySelector("span").innerHTML = "Output Pin " + oidx;
        getOutputsToggleButton(newOutput).id = "out_" + oidx;
        var newInputBody = newOutput.querySelector("div.categoryBody div")
        copyInputHtml(newInputBody,json,oidx)
        formdiv.appendChild(newOutput)
    }
    outputTemplate.remove()

}
function    setFieldValue(field, value, fieldType="input"){
    if (value == undefined)
        value = ""
    var field= document.querySelector(fieldType +"[name='" + field + "']")
    if( field)
        field.value = value
    else
        console.log("Field " + fieldType  + ":" + field + " not found")
}
function setInputAttributeValue(field, attribute, value){
    var field= document.querySelector("input[name='" + field + "']")
    if( field)
        field.setAttribute(attribute, value)
    else
        console.log("Field " + fieldType  + ":" + field + " not found")
}


const initIdx = -10
function buildScheduleString(a,max){
    rc= ""
    if( a.length == 0)
        return ""
    if( a.length == max)
        return "*"
    start= initIdx
    last = initIdx
    for( idx=0; idx < a.length; idx++){

        if( last+1 == a[idx]){
            if( start == initIdx){
                start = a[idx] 
                rc += "-"
            }     
        }
        else{
          if( start == initIdx && rc != "")
                rc += ","          
          start = initIdx
        }
            

        if( start == initIdx ||( idx == a.length -1))
           rc += a[idx]           
        last = a[idx]
    }
    return rc;
}
function buildConfigHtml(json){
    let hourPatternA = "(\\s*(1?[0-9])|(2?[0-3][0-9])\\s*)" // all numbers between 0 and 23 with leading and trailing separators
    let otherPatternA = "(\\s*([0-5]?[0-9])\\s*)"// all numbers between 0 and 59 with leading and trailing separators
    // One or more A or A-A separated by comma
    const allPattern = "^(((A-A?|A),)*(A-A?|A)|*)$" // A stands for either hourPatternA or hourPatternB
        if( json && json.builddate)
            document.getElementById( "BUILD_DATE").innerText = "Build date: " + json.builddate;
        if( json ){
            if( json.network != undefined){
                setFieldValue( 'hostname',json.network.hostname );
                setFieldValue( 'sslhost',json.network.sslhost );
                setFieldValue( 'sslhostkey',json.network.sslhostkey );
                setFieldValue( 'sslca',json.network.sslca );
                setFieldValue( 'logdestination',json.network.logdestination );
            }else{
                throw new Error("Invalid JSON file!!!");
            }
            if( json.mqtt != undefined){   
                setFieldValue( 'mqtturl',json.mqtt.mqtturl );
                setFieldValue( 'username',json.mqtt.username );
                setFieldValue( 'password',json.mqtt.password );
                setFieldValue( 'authenticationMethod',json.mqtt.authenticationMethod, "select" );
                setFieldValue( 'topic',json.mqtt.topic );
            }
            if( json.schedule != undefined){
                setFieldValue( 'hour',buildScheduleString(json.schedule.hour,24 ));
                setFieldValue( 'minute',buildScheduleString(json.schedule.minute,60 ) );
                setFieldValue( 'second',buildScheduleString(json.schedule.second,60 ) );
            }
        }

        setInputAttributeValue('hour', 'pattern', allPattern.replaceAll( "A",hourPatternA))
        setInputAttributeValue('minute', 'pattern', allPattern.replaceAll( "A",otherPatternA))
        setInputAttributeValue('second', 'pattern', allPattern.replaceAll( "A",otherPatternA))
        var noo = document.querySelectorAll("div.categoryBody div")
        noo.forEach( (e)=>{
            if(e.parentNode && e.parentNode.id && e.parentNode.id=='nooutBody')
                copyInputHtml(e,json,0xFF);
        })
        copyOutputsHtml(json);
        var inputTemplate = document.querySelector("[id='inputTemplate']")
        inputTemplate.remove();
        if( json && json.outputs != undefined){
            json.outputs.forEach(output=>{
                setFieldValue('type_' + output.port,output.type, "select")
            })
        }

}

function enableDisableUploadConfigButton(e){
            document.getElementById("importConfigButton").disabled = ( document.getElementById("importConfig").files.size  == 0 );
}
function downloadConfiguration(){
    json = JSON.stringify(buildJsonFromHtml());

    const jsonData = new Blob([json], { type: 'application/json' }); 
    const jsonContentURL = URL.createObjectURL(jsonData);
    const element = document.createElement("a");
    element.setAttribute("href",jsonContentURL);
    element.setAttribute("download", "plscount.json");
    element.style.display = "none";
    document.body.appendChild(element);
    element.click();
    document.body.removeChild(element);

}
lastCountIsRunning = false;
function updateLastCount(){
    if(lastCountIsRunning)
        return
    lastCountIsRunning = true;
    getJSON("/api/status",(err,result)=>{
        if(err == null){
            result.forEach( s=>{
                divObj = document.querySelector("div[id='lastCount_" + s.output + "_" + s.input + "']");
                if( divObj){
                    if( s.lastSecond != 0 ){
                        minutes = Math.trunc((Date.now()/1000 - s.lastSecond)/60 )
                        if( minutes == 0 )
                            divObj.innerHTML = "<p style='color:green'> Last Count: " + s.last +  ", A few seconds ago</p>"
                        else if( minutes < 60 )
                            divObj.innerHTML = "<p style='color:black'> Last Count: " + s.last +  ", " + Math.trunc((Date.now()/1000 - s.lastSecond)/60 )+ " min ago</p>"
                        else
                            divObj.innerHTML= "<p style='color:red'>No pulse since 1 or more hours</p>"
                    }
                    else
                        divObj.innerText = ""
                }
                    
            })
        }
        lastCountIsRunning = false;
        previousResult = result;
    })       
}

document.addEventListener("DOMContentLoaded", (event) => {
    updateLastCount();
    var intervalId = window.setInterval(updateLastCount, 1000);
  if( localStorage.getItem("importJSON") != null){
    newConfig = localStorage.getItem("importJSON");
    localStorage.removeItem("importJSON");
    try {
        buildConfigHtml(JSON.parse(newConfig));
    }
    catch(e){
        window.alert( e);
        window.location.reload();
    }
  }
  else
    getJSON("/api/config",(err,result)=>{
        if(err == null)
            buildConfigHtml(result)
    })
  const form = document.getElementById("form");
  form.removeEventListener("submit", sendConfig);
  form.addEventListener("submit", sendConfig);

  document.getElementById('importConfigButton').addEventListener('click', function(e) {
    var file = document.getElementById("importConfig").files[0];
    if (file) {
        var reader = new FileReader();
        reader.readAsText(file, "UTF-8");
        reader.onload = function (evt) {
            localStorage.setItem("importJSON" ,evt.target.result);
            window.location.reload();
        }
        reader.onerror = function (evt) {
            window.alert("Unable to import Config");
        }
    }
    });
  document.getElementById('resetTicks').addEventListener('click', sendResetTicks);

  document.getElementById("importConfig").removeEventListener('change',enableDisableUploadConfigButton);
  document.getElementById("importConfig").addEventListener('change',enableDisableUploadConfigButton );
});


