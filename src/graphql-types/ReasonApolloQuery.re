module type InternalConfig = {let apolloClient: ApolloClient.generatedApolloClient;};

module QueryFactory = (InternalConfig: InternalConfig) => {
  external castResponse : string => {. "data": Js.Json.t} = "%identity";
  external asJsObject : 'a => Js.t({..}) = "%identity";
  [@bs.module] external gql : ReasonApolloTypes.gql = "graphql-tag";
  [@bs.module] external shallowEqual : (Js.t({..}), Js.t({..})) => bool = "fbjs/lib/shallowEqual";
  type response =
    | Loading
    | Loaded(Js.Json.t)
    | Failed(string);
  type state = {
    response,
    variables: Js.Json.t
  };
  type action =
    | Result(string)
    | Error(string);
  let sendQuery = (~query, ~reduce) => {
    let _ =
      Js.Promise.(
        resolve(
          InternalConfig.apolloClient##query({
            "query": [@bs] gql(query##query),
            "variables": query##variables
          })
        )
        |> then_(
             (value) => {
               reduce(() => Result(value), ());
               resolve()
             }
           )
        |> catch(
             (_value) => {
               reduce(() => Error("an error happened"), ());
               resolve()
             }
           )
      );
    ()
  };
  let component = ReasonReact.reducerComponent("ReasonApollo");
  let make = (~query as q, children) => {
    ...component,
    initialState: () => {response: Loading, variables: q##variables},
    reducer: (action, state) =>
      switch action {
      | Result(result) =>
        let typedResult = castResponse(result)##data;
        ReasonReact.Update({...state, response: Loaded(typedResult)})
      | Error(error) => ReasonReact.Update({...state, response: Failed(error)})
      },
    willReceiveProps: ({state, reduce}) =>
      if (! shallowEqual(asJsObject(q##variables), asJsObject(state.variables))) {
        sendQuery(~query=q, ~reduce);
        state
      } else {
        state
      },
    didMount: ({reduce}) => {
      sendQuery(~query=q, ~reduce);
      ReasonReact.NoUpdate
    },
    render: ({state, reduce}) =>
      children(state.response, q##parse, () => sendQuery(~query=q, ~reduce))
  };
};
