### Storage of String Values

String values stored in a DXF file can be expressed in plain ASCII, UTF-8, CIF (Common Interchange Format), 
and MIF (Maker Interchange Format) formats. The UTF-8 format is only supported in the AutoCAD 2007 DXF and 
later file formats. When the AutoCAD program writes a DXF file, the format in which string values are written 
is determined by the DXF file format chosen. String values are written out in these formats:

AutoCAD 2007 DXF and later format - UTF-8
AutoCAD 2004 DXF and earlier format - Plain ASCII and CIF
String values containing Unicode characters are represented with control character sequences.

For example, "TEST\U+7F3A\U+4E4F\U+89E3\U+91CA\U+6B63THIS\U+56FE"

String values can be stored with these dxf group codes:

	0 - 9
	100 - 101
	300 - 309
	410 - 419
	430 - 439
	470 - 479
	999 - 1003