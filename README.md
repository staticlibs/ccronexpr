Tools for working with BLOBs in different databases
===================================================

Library allows to work with BLOBs in streaming mode (particularly - write to BLOB as to `OutputStream`)
in different databases using the same API (created initially for PostgreSQL). Supports transparent compression
of BLOB data.

Depends on [spring-jdbc](http://static.springsource.org/spring/docs/3.0.x/spring-framework-reference/html/jdbc.html).

Library is available in [Maven central](http://repo1.maven.org/maven2/com/alexkasko/springjdbc/):

    <dependency>
        <groupId>com.alexkasko.springjdbc</groupId>
        <artifactId>blob-tool</artifactId>
        <version>1.0</version>
    </dependency>

Javadocs for the latest release are available [here](http://alexkasko.github.com/blob-tool/javadocs).

Usage example with Spring and PostgreSQL
----------------------------------------

Blob tool setup:

    @Bean
    public BlobTool blobTool() {
        return new PostgresBlobTool(dataSource(), compressor());
    }

Create BLOB and write data into it:

    OutputStreamBlob blob = null;
    try {
        // create server-side BLOB
        blob = blobTool.create();
        // use OutputSteam from BLOB
        createHugeReport(blob.outputStream());
        // save BLOB id in application table for subsequent use
        save(new AppObj(foo, bar, blob.getId()));
    } finally {
        // release DB resources
        closeQuietly(blob);
    }

Read data from BLOB:

    InputStreamBlob blob = null;
    try {
        // open server-side BLOB by ID
        blob = blobTool.load(id);
        // read stored data
        processStoredData(blob.inputStream());
    } finally {
        // release DB resources
        closeQuietly(blob);
    }

Working with BLOBs through their IDs
-------------------------------------

Library was initially created for PostgreSQL and uses it's approach when `long` BLOB ID (OID in postgres) is stored in
application table and blob data should be accessed using this ID explicitly. For other RDBMSes that doesn't allow
direct control over BLOB ID (when BLOB itself is "stored" in application table) library uses additional tables like this:

    create table blob_storage (
        id int primary key,
        data blob
    );

Streaming BLOBs in JDBC
-----------------------

This library uses BLOBs exclusively in streaming mode without loading them into memory on any operation.

Most RDBMSes support streaming read of BLOB data through JDBC API. But proper streaming write to BLOBs is less supported.
To write into server-side BLOBs in streaming mode using `OutputStream` you may use this JDBC method
[Blob#setBinaryStream](http://docs.oracle.com/javase/6/docs/api/java/sql/Blob.html#setBinaryStream%28long%29).

Besides its usage is not intuitive (`set` method returns `OutputStream`):

    Blob b = ...
    OutputStream blobStream = b.setBinaryStream(1);

it's also (among popular RDBMSes) implemented properly (with real server-side BLOB) only in PostgreSQL and in Oracle.

This library contains different implementations for RDBMSes that support server side streaming
write and for those that does not. Fallback implementation (for MSSQL, MySQL etc.) uses temporary file for writing to BLOB
as to OutputStream and then flushes data to server. This method may not suit highload applications, but may be useful e.g.
for using [H2 database](http://www.h2database.com/html/main.html) instead of PostgreSQL or Oracle in testing/prototyping.

BLOBs compression
-----------------

Library supports transparent compression of BLOB data. Supported compression methods:"

 - `GzipCompressor`: GZIP compression using JDK GZIP implementation
 - `SnappyCompressor`: [Snappy](http://en.wikipedia.org/wiki/Snappy_%28software%29) compression
 - `XzCompressor`: [LZMA2](http://en.wikipedia.org/wiki/Xz) compression
 - `NoCompressor`: compression is disabled

These libraries are required for Snappy and XZ compression:

    <dependency>
        <groupId>org.iq80.snappy</groupId>
        <artifactId>snappy</artifactId>
        <version>0.3</version>
    </dependency>
    <dependency>
        <groupId>org.tukaani</groupId>
        <artifactId>xz</artifactId>
        <version>1.0</version>
    </dependency>

License information
-------------------

This project is released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)

Changelog
---------

**1.0** (2013-03-15)

 * initial public version