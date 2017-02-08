var child_process = require('child_process');
var path = require('path');

function withService(callback) {
	// Start a new process with our service code.
	var p = child_process.fork(path.join(__dirname, 'service.js'));

	var done = function() {
		p.kill();
	}

	done.process = p;

	p.on('message', function(m) {
		// When the service process has started and notifies us that it
		// is ready, we call the callback so that the test code can
		// proceed.
		callback(null, done);
	});

	p.on('exit', function() {
		// Because there is no way to shut down a connection, the node
		// process keeps running after we're done with our testing. The
		// way we handle this, then, is that when a test is done using
		// the service, we kill the process that we started. And once
		// that process has exited, we kill the current process.
		process.exit();
	});
}

module.exports = withService;
