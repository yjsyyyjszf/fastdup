/* Copyright (c) 2008, John Brooks <special@dereferenced.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of dereferenced.net, FastDup, or its contributors may not be
 *       used to endorse or promote products derived from this software without
 *       specific prior written permission, with the exception of simple
 *       attribution.
 *     * Redistribution, in whole or in part, with or without modification, for
 *       or as part of a commercial product or venture is prohibited without
 *       explicit written consent from the author.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BROOKS ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "main.h"
#include <getopt.h>

std::map<off_t,FileReference*> SizeMap;
std::vector<FileReference*> SizeDups;
int DupeCount = 0, DupeSetCount = 0, FileCount = 0;
off_t ScannedSize = 0;
bool FileErrors = false;
bool Interactive = false;

int treecount = 0;
char **scantrees = NULL;

static void ShowHelp(const char *bin);

static bool ReadOptions(int argc, char **argv)
{
	Interactive = isatty(fileno(stdout));
	
	char opt;
	while ((opt = getopt(argc, argv, "ibh")) >= 0)
	{
		switch (opt)
		{
			case 'i':
				Interactive = true;
				break;
			case 'b':
				Interactive = false;
				break;
			case 'h':
			default:
				ShowHelp(argv[0]);
				return false;
		}
	}
	
	if (optind >= argc)
	{
		ShowHelp(argv[0]);
		return false;
	}
	
	char cwd[PATH_MAX];
	char tmp[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX))
	{
		printf("Unable to get current directory: %s\n", strerror(errno));
		return false;
	}
	
	treecount = argc - optind;
	scantrees = new char*[treecount];
	for (int i = optind, j = 0; i < argc; i++, j++)
	{
		if (argv[i][0] != '/')
		{
			PathMerge(tmp, PATH_MAX, cwd, argv[i]);
			int len = strlen(tmp) + 2;
			scantrees[j] = new char[len];
			PathResolve(scantrees[j], len, tmp);
		}
		else
		{
			int len = strlen(argv[i]);
			scantrees[j] = new char[len + 2];
			PathResolve(scantrees[j], len + 2, argv[i]);
		}
		
		if (!DirectoryExists(scantrees[j]))
		{
			printf("Error: %s does not exist or is not a directory\n", argv[i]);
			return false;
		}
	}
	
	return true;
}

int main(int argc, char **argv)
{
	if (!ReadOptions(argc, argv))
		return EXIT_FAILURE;
	
	/* Initial scan - this step will recurse through the directory tree(s)
	 * and find each file we will be working with. These files are mapped
	 * by their size as this process runs through, so we will be provided
	 * with a list of files with the same sizes at the end. Those files
	 * are what we select for the deep comparison, which is where the magic
	 * really shows ;) */
	printf("Scanning for files...\n");
	double starttm = SSTime();
	for (int i = 0; i < treecount; i++)
		ScanDirectory(scantrees[i], strlen(scantrees[i]), NULL);
	
	if (SizeMap.empty())
	{
		printf("\nNo files found!\n");
		return EXIT_SUCCESS;
	}
	
	if (FileErrors && Interactive)
	{
		if (!PromptChoice("\nUnable to scan some files. Do you want to continue [y/n]? ", false))
			return EXIT_FAILURE;
	}
	
	SizeMap.clear();
	printf("Comparing %lu set%s of files...\n\n", SizeDups.size(), (SizeDups.size() != 1) ? "s" : "");
	
	for (std::vector<FileReference*>::iterator it = SizeDups.begin(); it != SizeDups.end(); ++it)
	{
		DeepCompare(*it);
	}
	double endtm = SSTime();
	
	printf("Found %d duplicate%s of %d file%s\n", DupeCount - DupeSetCount, (DupeCount - DupeSetCount != 1) ? "s" : "", DupeSetCount, (DupeSetCount != 1) ? "s" : "");
	printf("Scanned %sB in %d file%s in %.3f seconds\n", ByteSizes(ScannedSize).c_str(), FileCount, (FileCount != 1) ? "s" : "", endtm - starttm);
	
	return EXIT_SUCCESS;
}

static void ShowHelp(const char *bin)
{
	printf(
		"fastdup " FASTDUP_VERSION " - http://dev.dereferenced.net/fastdup/\n\n"
		"Usage: %s [options] directory [directory..]\n"
		"Options:\n"
		"    -i                          Enable interactive prompts (default on terminals)\n"
		"    -b                          Disable interactive prompts (batch mode)\n"
		"    -h                          Show help and options\n"
		"\n", bin
	);
}
