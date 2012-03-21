# node-dbus  
node-dbus is a D-Bus binding for node has the following capabilities:
* call D-Bus methods
* receive D-Bus signals
* export D-Bus services

## How To Build
To build, do:
      ```
	    node-waf configure build
       ```

## Test node-dbus
### To test method call
    Run the sample D-Bus service first:
	     ```
       python examples/test-service.py
       ```
     Then run the test script in node:
       ```
       node examples/test_method.js
       ```

### To test signal receipt
    Run the sample D-Bus signal service first:
	     ```
	     python examples/example-signal-emitter.py
       ```

    Then run the test script in node:
	     ```
	     node examples/test_signal2.js
       ```

### To test create D-Bus service
    Run the sample service script file:
       ```
       node examples/test_service1.js
       ```

    Then run the service client teset script:
       ``` 
	     node examples/test_service1_method.js
       ```

## License 

(The MIT License)

Copyright (c) 2012 

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

