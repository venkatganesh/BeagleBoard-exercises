// From Getting Started With node.js and socket.io 
// http://codehenge.net/blog/2011/12/getting-started-with-node-js-and-socket-io-v0-7-part-2/
"use strict";

var http = require('http'),
    url = require('url'),
    fs = require('fs'),
    exec = require('child_process').exec,
    server,
    connectCount = 0;	// Number of connections to server
    var color = [0, 0, 0];	// Color of next LED

    var rgbPath    = "/sys/firmware/lpd8806/device";
    var pinMuxPath = "/sys/kernel/debug/omap_mux";
    var errCount = 0;	// Counts the AIN errors.

// Initialize various IO things.
function initIO() {
    // Make sure gpio 7 is available.
    var gpio = 7;
    fs.writeFile("/sys/class/gpio/export", gpio);
}

initIO();

server = http.createServer(function (req, res) {
// server code
    var path = url.parse(req.url).pathname;
    console.log("path: " + path);
    switch (path) {
    case '/':
        res.writeHead(200, {'Content-Type': 'text/html'});
        res.write('<h1>Hello!</h1>Try<ul><li><a href="/tree.html">Christmas Tree Demo</a></li></ul>');

        res.end();
        break;

    default:		// This is so all the files will be sent.
        fs.readFile(__dirname + path, function (err, data) {
            if (err) {return send404(res); }
//            console.log("path2: " + path);
            res.write(data, 'utf8');
            res.end();
        });
        break;

    }
});

var send404 = function (res) {
    res.writeHead(404);
    res.write('404');
    res.end();
};

server.listen(8081);
console.log("Listening on 8081");

// socket.io, I choose you
var io = require('socket.io').listen(server);
io.set('log level', 2);

// on a 'connection' event
io.sockets.on('connection', function (socket) {
    var frameCount = 0;	// Counts the frames from arecord
    var lastFrame = 0;	// Last frame sent to browser
    console.log("Connection " + socket.id + " accepted.");
//    console.log("socket: " + socket);

    // now that we have our connected 'socket' object, we can 
    // define its event handlers

    // Make sure some needed files are there
    // The path to the analog devices changed from A5 to A6.  Check both.
    var ainPath = "/sys/devices/ocp.2/helper.14/";
    if(!fs.existsSync(ainPath)) {
	// Use device tree to make path appear.
        fs.writeFileSync("/sys/devices/bone_capemgr.9/slots", "cape-bone-iio");
    }    

    // Send value every time a 'message' is received.
    socket.on('ain', function (ainNum) {
        fs.readFile(ainPath + "AIN" + ainNum, 'base64', function(err, data) {
            if(err && errCount++<5) console.log("AIN read error"); //throw err;
            socket.emit('ain', data);
//            console.log('emitted ain: ' + data);
        });
    });

    socket.on('gpio', function (gpioNum) {
        var gpioPath = "/sys/class/gpio/gpio" + gpioNum + "/value";
        fs.readFile(gpioPath, 'base64', function(err, data) {
            if (err) throw err;
            socket.emit('gpio', data);
//            console.log('emitted gpio: ' + data);
        });
    });

    socket.on('led', function (ledNum) {
        var ledPath = "/sys/class/leds/beaglebone:green:usr";
	var ledPathBright = ledPath + ledNum + "/brightness";
//        console.log('LED: ' + ledPath);
        fs.readFile(ledPathBright, 'utf8', function (err, data) {
            if(err) throw err;
            data = data.substring(0,1) === "1" ? "0" : "1";
//            console.log("LED%d: %s", ledNum, data);
            fs.writeFile(ledPathBright, data);
        });
    });
    socket.on('trigger', function(trig) {
//	console.log('trigger: ' + trig);
	if(trig) {
		trigger("heartbeat mmc0 cpu0 none");
	} else {
		trigger("none none none none");
	}
    });
    function trigger(arg) {
        var ledPath = "/sys/class/leds/beaglebone:green:usr";
//	console.log("trigger: " + arg);
	arg = arg.split(" ");
	for(var i=0; i<4; i++) {
//	    console.log(" trigger: ", arg[i]);
	    fs.writeFile(ledPath + i + "/trigger", arg[i])
	}
    }

    socket.on('slider', function(slideNum, value) {
//	console.log('slider' + slideNum + " = " + value);
	color[slideNum] = value;
//	console.log('color: ' + color);
	var i;
	for(i=0; i<160; i++) {
          fs.writeFileSync(rgbPath + "/rgb", 
	    color[0]+' '+color[1]+' '+color[2]+' '+i);
	  }
	fs.writeFileSync(rgbPath + "/rgb", '\n');
    });

    socket.on('rgb', function(rgbString) {
	console.log('rgb: ' + rgbString);
        rgbString = rgbString.split(" ");
        fs.writeFile(rgbPath + "/grb", 
	  rgbString[1]+' '+rgbString[0]+' '+rgbString[2]);
    });

//	The 'data' interface isn't stable.
//	This uses the 'data' interface to address all the LEDs at once.
//	It appears very fast, but seems to hang everything.
    socket.on('getData', function(count) {
      var LEDout = [];
      console.log('getData: ' + count);
      var LEDpath = '/sys/firmware/lpd8806/device';
      fs.readFile(LEDpath + "/data", 'ascii', function(err, data) {
	    data = data.split('\n');
            if(err) throw err;
            socket.emit('led', data);
            console.log('emitted LEDdata: ');
	    for(var j=90; j>88; j--) {
	    LEDout = [];
	    var on = j;
            for(var i=0; i<count; i++) {
	      if(i == on) {
		LEDout += "127 127 127 ";
	      } else {
		LEDout += "0 0 0 ";
	      }
	    }
	    LEDout += '\n';
	    console.log("LEDout: " + LEDout);
	    fs.writeFileSync(LEDpath + "/data", LEDout);
	    }
        });

    });

    socket.on('disconnect', function () {
        console.log("Connection " + socket.id + " terminated.");
        connectCount--;
        if(connectCount === 0) {
        }
        console.log("connectCount = " + connectCount);
    });

    connectCount++;
    console.log("connectCount = " + connectCount);
});

