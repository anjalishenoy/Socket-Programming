peer

====

Usage ./peer \<IP\> \<Port of Remote Machine\> \<Port of Your Machine\>

Basic filesharing application
 To enable two clients to:
-Know list of files on each other's machines in designate shared folders 
-Upload a file to each other
-Download a file to each other
-Periodically check for changes in the shared folder
-Application level error checking using MD5 checksum

1. An "IndexGet" request which can request different styles of the same index of the shared folder on the other client as listed below.	

a. A "ShortList" request indicating that the requesting client wants to know only the names files chosen from the time-stamps specified by the requesting client, i.e., the client only wishes to learn about a few files. 	

E.g., Sample request: IndexGet ShortList starting-time-stamp ending-time-stamp 	

The response includes the "names", "sizes", "last modified" time-stamp of the files 	

b. A "LongList" request indicating that the requesting client wants to know the entire listing of the directory of the shared folder including the "names", "sizes", "last modified timestamp" 	

c. A "RegEx" request indicating that the requesting client wants to know the list of files that contain the regular expression pattern in their file names. The response includes all the file names which "contain" the regular expression pattern in their names. 	

E.g., IndexGet RegEx '*mp3'

2. A "FileHash" request indicates that the client to enable the client to check if any file's content has changed. Two types of "FileHash" are supported: 	

a. A "Verify" request which gives the name of the file that the client wants the hash for. The response contains the MD5 hash of the file and the name of the file and last modified time stamp. 	

E.g., FileHash Verify Name-of-file 	

b.A "Check All" request which is used to periodically check for modifications in the file. The response includes the hashes of all the files, their names and the last modified time stamp. 	

E.g., FileHash CheckAll 	

3. A "FileDownload" request, which includes the name of the file that the client wants to download. The response includes the File, the file name, the file size, the MD5 hash and the time-stamp when the file was last modified.

E.g., FileDownload Name-of-file 	

4. A "FileUpload" request, which includes the name and size of the file that the client wants to upload.The other client can either end a"FileUploadDeny"or "FileUploadAllow" response. The client uploads the file,its md5 hash and the time-stamp if it receives a"FileUploadAllow" response. If it gets a "FileUpload Deny" then the client goes back to listening for other requests. 

E.g., FileUpload Name-of-file 	
