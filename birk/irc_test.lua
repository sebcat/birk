local irc = require "birk.irc"

function same(actual, expected)
  if type(expected) ~= type(actual) then
    error(type(expected).." ~= "..type(actual))
  end

  local stract = tostring(actual)
  local strexp = tostring(expected)
  if type(expected) ~= "table" then
    if stract ~= strexp then
      error(stract.." ~= "..strexp)
    else
      return
    end
  end

  local keys = {}
  for k,v in pairs(actual) do
    local expval = expected[k]
    if expval == nil or same(v, expval) then
      error("unexpected key: "..tostring(k))
    end
    keys[k] = true
  end

  for k, _ in pairs(expected) do
    if not keys[k] then error("missing expected key: "..tostring(k)) end
  end
end

function not_same(t1, t2)
  ok, _ = pcall(same, t1, t2)
  if ok then error("t1 == t2") end
end

function test_message(msg, expected)
  same(irc.message(msg), expected)
end

-- basic sanity checks
same({}, {})
same({a="b"}, {a="b"})
same({a={}}, {a={}})
same({a=true}, {a=true})
same("", "")
same(0, 0)
same(nil, nil)
not_same({}, "")
not_same({}, nil)
not_same({}, 0)
not_same({}, {a={}})
not_same({a={}}, {})
not_same({a=true}, {a=false})
not_same({{}}, {})
not_same("nil", nil)
not_same("0", 0)

-- some RFC1459 examples
test_message("PASS secretpasswordhere", {
    cmd="PASS",
	params={"secretpasswordhere"}})
test_message("NICK Wiz", {
    cmd="NICK",
    params={"Wiz"}})
test_message(":WiZ NICK Kilroy", {
    name="WiZ",
	cmd="NICK",
	params={"Kilroy"}})
test_message("USER guest tolmoon tolsun :Ronnie Reagan", {
	cmd="USER",
	params={"guest", "tolmoon", "tolsun", "Ronnie Reagan"}})
test_message(":testnick USER guest tolmoon tolsun :Ronnie Reagan", {
	name="testnick",
	cmd="USER",
	params={"guest", "tolmoon", "tolsun", "Ronnie Reagan"}})
test_message("SERVER test.oulu.fi 1 :[tolsun.oulu.fi] Experimental server", {
	cmd="SERVER",
	params={"test.oulu.fi", "1", "[tolsun.oulu.fi] Experimental server"}})
test_message(":tolsun.oulu.fi SERVER csd.bu.edu 5 :BU Central Server", {
	name="tolsun.oulu.fi",
	cmd="SERVER",
	params={"csd.bu.edu", "5", "BU Central Server"}})
test_message("OPER foo bar", {
	cmd="OPER",
	params={"foo", "bar"}})
test_message("QUIT :Gone to have lunch", {
	cmd="QUIT",
	params={"Gone to have lunch"}})
test_message("SQUIT tolsun.oulu.fi :Bad Link ?", {
	cmd="SQUIT",
	params={"tolsun.oulu.fi", "Bad Link ?"}})
test_message(":Trillian SQUIT cm22.eng.umd.edu", {
	name="Trillian",
	cmd="SQUIT",
	params={"cm22.eng.umd.edu"}})
test_message("JOIN #foobar", {
	cmd="JOIN",
	params={"#foobar"}})
test_message("JOIN &foo fubar", {
	cmd="JOIN",
	params={"&foo", "fubar"}})
test_message("JOIN #foo,&bar fubar", {
	cmd="JOIN",
	params={"#foo,&bar", "fubar"}})
test_message("JOIN #foo,#bar fubar,foobar", {
	cmd="JOIN",
	params={"#foo,#bar", "fubar,foobar"}})
test_message("JOIN #foo,#bar", {
	cmd="JOIN",
	params={"#foo,#bar"}})
test_message(":WiZ JOIN #Twilight_zone", {
	name="WiZ",
	cmd="JOIN",
	params={"#Twilight_zone"}})


-- some other tests
test_message(":WiZ!foo@server JOIN #foo", {
	name="WiZ",
	user="foo",
	host="server",
	cmd="JOIN",
	params={"#foo"}})
test_message("REHASH", {cmd="REHASH"})
