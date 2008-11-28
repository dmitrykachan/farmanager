BEGIN{
  print "Rebuild dependencies..." > "/dev/stderr"
  if(out == "")
    out="Release.vc";

  if(length(ENVIRON["FARSYSLOG"]) > 0)
    objdir="objlog";
  else
    objdir="obj"
}
{
  i = split($0, a, ".");
  filename = a[1];
  for (j=2; j < i; j++)
    filename = filename "." a[j]
  ext = a[i];

  if(ext == "cpp" || ext == "c")
    ext="obj";
  if(ext == "rc")
    ext="res";
  if(ext == "hpp")
  {
    ext="hpp";
    print ".\\" filename "." ext " : \\";
  }
  else
  {
    print ".\\" out "\\" objdir "\\" filename "." ext " : \\";
    print "\t\".\\" $0 "\"\\";
  }
  while((getline lnsrc < ($0)) > 0)
  {
    if(substr(lnsrc,1,length("#include \"")) == "#include \"")
    {
      #print lnsrc > "/dev/stderr"
      lnsrc=gensub(/^#include[ \t]?\"([^\"]+)\"/, "\\1", "g", lnsrc);
      if(lnsrc != "")
        print "\t\".\\" lnsrc "\"\\";
    }
  }
  print "\n\n"
}
