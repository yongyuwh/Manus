## Synopsis

This library is part of the Manus SDK and provides functionality to communicate with the Manus Glove and the Manus Interface. Currently only communication with the Manus Glove is implemented.

## Usage

To communicate with the Manus Glove the SDK has to be initialized with ManusInit() after which the current state of a glove can be retrieved with ManusGetData().

When no longer using the SDK ManusExit() should be called so that the SDK can safely shut down.

## Code Example

A minimal program to retrieve the data from the left Manus Glove looks like this:

	ManusInit();

	while (true)
	{
		GLOVE_DATA data;
		if (ManusGetData(GLOVE_LEFT, &data) == MANUS_SUCCESS)
		{
			// The data structure now contains the raw glove data
		}
		else
		{
			// The requested glove is not connected or an error occured
		}

		GLOVE_SKELETAL model;
		ManusGetSkeletal(GLOVE_LEFT, &model)
		{
			// The model structure now contains the skeletal model for the hand
		}
		else
		{
			// The requested glove is not connected or an error occured
		}
	}
	
	ManusExit();

## Documentation

The full documentation is available at [labs.manusmachina.com](http://labs.manusmachina.com/).

## License

The Manus SDK is licensed under LGPL and the Manus Interface is licensed under GPL.
